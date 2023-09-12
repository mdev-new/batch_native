#include "audio.hpp"

namespace Audio
{
	Audio::Audio()
		:decoder{}
	{}

	Audio::~Audio() {
		ma_decoder_uninit(&this->decoder);
	}

	void Audio::Wait() const
	{
		if (!silence)
		{
			std::unique_lock<std::mutex> lock{ mutex };
			cvDone.wait(lock, [this] { return silence; });
		}
	}

	bool Audio::IsPlaying() const
	{
		return !silence;
	}

	void Audio::Stop()
	{
		stoplater = true;
	}

	void Audio::SetEndCallback(std::function<void()> callback)
	{
		onFinishCallback = std::move(callback);
	}

	void Audio::Rewind()
	{
		stoplater = false;
		ma_decoder_seek_to_pcm_frame(&decoder, 0);
	}

	unsigned Audio::Data(void* output, unsigned frameCount)
	{
		const auto framesDecoded =
			ma_decoder_read_pcm_frames(&decoder, output, frameCount);

		bool silenceNow = framesDecoded == 0 || stoplater;
		if (silenceNow && !silence)
		{
			{
				std::lock_guard<std::mutex> lock{ mutex };
				silence = true;
			}
			EndedPlayingCallback();
		}
		else
		{
			silenceNow = silenceNow;
		}

		return unsigned(framesDecoded);
	}

	void Audio::EndedPlayingCallback()
	{
		if (onFinishCallback)
			onFinishCallback();
		cvDone.notify_all();
	}

	AudioFile::AudioFile(std::string filename)
	{
		ma_decoder_config cfg = ma_decoder_config_init(ma_format_f32, 2, 44100); // TODO read from device
		ma_decoder_init_file(filename.c_str(), &cfg, &decoder);
	}

	AudioFile::AudioFile() {
		this->cfg = ma_decoder_config_init(ma_format_f32, 2, 44100); // TODO read from device
	}

	void AudioFile::SetFile(std::string& str) {
		ma_decoder_init_file(str.c_str(), &this->cfg, &decoder);
	}

	AudioMemView::AudioMemView(const void* data, std::size_t size)
	{
		ma_decoder_config cfg = ma_decoder_config_init(ma_format_f32, 2, 44100); // TODO read from device
		ma_decoder_init_memory(data, size, &cfg, &decoder);
	}

	SndOutStream::SndOutStream(const SndOutStreamConfig& config)
		: dev{}, devcfg(MakeMAConfig(config))
	{
		ma_device_init(nullptr, &devcfg, &dev);
	}

	SndOutStream::~SndOutStream()
	{
		ma_device_uninit(&dev);
	}

	void SndOutStream::Start()
	{
		PlayImpl();
	}

	void SndOutStream::StopAll()
	{
		stoplater = true;
	}

	void SndOutStream::StopStream()
	{
		StopAll();
		ma_device_stop(&dev);
	}

	void SndOutStream::Play(Audio& audio)
	{
		audio.Rewind();
		audio.silence = false;
		audios.push_back(&audio);
		PlayImpl();
	}

	void SndOutStream::Wait() const
	{
		if (!silence)
		{
			std::unique_lock<std::mutex> lock{ mutex };
			cvDone.wait(lock, [this] { return silence; });
		}
	}

	void SndOutStream::SetVol(float val)
	{
		vol = val;
	}

	void SndOutStream::DataCallback(ma_device* dev, void* output, const void* input, ma_uint32 frameCount)
	{
		(void)input;
		auto self = static_cast<SndOutStream*>(dev->pUserData);
		self->DataCallbackImpl(output, frameCount);
	}

	void SndOutStream::DataCallbackImpl(void* output, ma_uint32 frameCount)
	{
		std::size_t buffer_size_frames =
			frameCount * devcfg.playback.channels;

		framesBuf.resize(buffer_size_frames);
		float32* audio_output = framesBuf.data();
		auto fOutput = static_cast<float32*>(output);
		std::memset(fOutput, 0, buffer_size_frames * sizeof(float32));

		for (auto* audio : audios)
		{
			const auto framesDecoded = audio->Data(audio_output, frameCount);
			for (unsigned i = 0;
				i < framesDecoded * devcfg.playback.channels; ++i)
				fOutput[i] += vol * audio_output[i];
		}

		// Remove all finished audios:
		auto toRemove = stoplater
			? audios.begin()
			: std::remove_if(audios.begin(), audios.end(),
				[](const Audio* a) {
					return !a->IsPlaying();
				});
		audios.erase(toRemove, audios.end());
		if (audios.empty() && !silence)
		{
			{
				std::lock_guard<std::mutex> lock{ mutex };
				silence = true;
			}
			EndedPlayingCallback();
		}
		silence = audios.empty();
	}

	void SndOutStream::EndedPlayingCallback()
	{
		stoplater = false;
		cvDone.notify_all();
	}

	ma_device_config SndOutStream::MakeMAConfig(const SndOutStreamConfig& osCfg)
	{
		ma_device_config cfg = ma_device_config_init(ma_device_type_playback);
		cfg.playback.format = ma_format_f32;
		cfg.playback.channels = osCfg.channels;
		cfg.sampleRate = osCfg.sampleRate;
		cfg.dataCallback = DataCallback;
		cfg.pUserData = this;
		cfg.periodSizeInMilliseconds = osCfg.bufSizeMS;
		return cfg;
	}

	void SndOutStream::PlayImpl()
	{
		silence = false;
		if (ma_device_get_state(&dev) != MA_STATE_STOPPED)
			return;
		ma_device_start(&dev);
	}

	SndOutStream& operator<<(SndOutStream& aout, Audio& a)
	{
		aout.Play(a);
		return aout;
	}
}