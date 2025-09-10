#pragma once

namespace az
{
	class FileAccessor;
}

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <azure/core/url.hpp>
#include "filereader.hpp"
#include "fileoutputstream.hpp"

namespace az
{
	class FileAccessor
	{
	public:
		virtual bool Exists() const = 0;
		virtual size_t GetSize() const = 0;
		virtual FileReader& OpenForReading() const = 0;
		virtual FileOutputStream& OpenForWriting() const = 0;
		virtual FileOutputStream& OpenForAppending() const = 0;
		virtual void Remove() const = 0;
		virtual void MkDir() const = 0;
		virtual void RmDir() const = 0;
		virtual size_t GetFreeDiskSpace() const = 0;
		virtual void CopyTo(const std::string& destUrl) const = 0;
		virtual void CopyFrom(const std::string& sourceUrl) const = 0;

		virtual ~FileAccessor() = 0;

	protected:
		FileAccessor(const Azure::Core::Url& url, const std::function<FileReader& (FileReader&&)>& registerReader, const std::function<FileOutputStream& (FileOutputStream&&)>& registerWriter);

		const Azure::Core::Url& GetUrl() const;
		bool HasDirUrl() const;
		void CheckUrl() const;
		virtual std::vector<std::string> UrlPathParts() const = 0;
		virtual void CheckFileUrl() const = 0;
		virtual void CheckDirUrl() const = 0;

		std::function<FileReader& (FileReader&&)> RegisterReader;
		std::function<FileOutputStream& (FileOutputStream&&)> RegisterWriter;

	private:
		Azure::Core::Url url;
		bool bHasDirUrl;
	};
}
