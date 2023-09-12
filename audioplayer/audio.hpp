#pragma once
#include <algorithm>
#include <condition_variable>
#include <string>
#include <cstring>
#include <exception>
#include <functional>
#include <mutex>
#include <utility>
#include <vector>


#include "miniaudio.h"


namespace Audio
{
	class Audio
	{
		friend class SndOutStream;

	public:
		void Wait() const;
		bool IsPlaying() const;
		void Stop();
		void SetEndCallback(std::function<void()> callback);
		~Audio();

	private:
		void Rewind();
		unsigned Data(void* output, unsigned frame_count);
		void EndedPlayingCallback();

		mutable std::mutex mutex{};
		mutable std::condition_variable cvDone{};
		std::function<void()> onFinishCallback{};
		bool silence{ true };
		bool stoplater{ false };

	protected:
		Audio();
		ma_decoder decoder;
	};


	class AudioFile final : public Audio
	{
	public:
		AudioFile(std::string filename);
		AudioFile();
		void SetFile(std::string &file);
	private:
		ma_decoder_config cfg;
	};


	class AudioMemView final : public Audio
	{
	public:
		AudioMemView(const void* data, std::size_t size);
	};


	struct SndOutStreamConfig
	{
		unsigned int sampleRate{ 44100 };
		unsigned int bufSizeMS{ 200 };
		unsigned short channels{ 2 };
	};


	class SndOutStream final
	{
		using float32 = float;
		static_assert(sizeof(float32) == 4, "Platform is not supported");

	public:
		explicit SndOutStream(const SndOutStreamConfig& config = SndOutStreamConfig{});
		~SndOutStream();

		SndOutStream(const SndOutStream&) = delete;
		SndOutStream(SndOutStream&&) = delete;

		SndOutStream& operator=(const SndOutStream&) = delete;
		SndOutStream& operator=(SndOutStream&&) = delete;

		void Start();
		void StopAll();
		void StopStream();
		void Play(Audio& audio);
		void Wait() const;
		void SetVol(float val);

	private:
		static void DataCallback(ma_device* dev, void* output, const void* input, ma_uint32 frameCount);
		void DataCallbackImpl(void* output, ma_uint32 frameCount);
		void EndedPlayingCallback();
		ma_device_config MakeMAConfig(const SndOutStreamConfig& sndoutstrcfg);
		void PlayImpl();

		ma_device dev;
		ma_device_config devcfg;
		mutable std::mutex mutex;
		mutable std::condition_variable cvDone;
		std::vector<Audio*> audios;
		std::vector<float32> framesBuf;
		float vol{ 1.0f };
		bool stoplater{ false };
		bool silence{ true };
	};

	SndOutStream& operator<<(SndOutStream& aout, Audio& a);
}