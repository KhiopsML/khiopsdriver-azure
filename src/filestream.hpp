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
		explicit operator void* () const;
		virtual void Close() = 0;

	protected:
		FileStream();

	private:
		void* handle;
	};
}
