#pragma once

namespace az
{
	class FileStream;
}

namespace az
{
	class FileStream
	{
	public:
		virtual ~FileStream();
		void* GetHandle() const;
		virtual void Close() = 0;

	protected:
		FileStream();

	private:
		void* handle;
	};
}
