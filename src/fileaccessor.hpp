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
#include "filewriter.hpp"
#include "fileappender.hpp"

namespace az
{
	class FileAccessor
	{
	public:
		virtual bool Exists() const = 0;
		virtual size_t GetSize() const = 0;
		virtual const std::unique_ptr<FileReader>& OpenForReading() const = 0;
		virtual const std::unique_ptr<FileOutputStream>& OpenForWriting() const = 0;
		virtual const std::unique_ptr<FileOutputStream>& OpenForAppending() const = 0;
		virtual void Remove() const = 0;
		virtual void MkDir() const = 0;
		virtual void RmDir() const = 0;
		virtual size_t GetFreeDiskSpace() const = 0;
		virtual void CopyTo(const std::string& destUrl) const = 0;
		virtual void CopyFrom(const std::string& sourceUrl) const = 0;

		virtual ~FileAccessor() = 0;

	protected:
		FileAccessor(const Azure::Core::Url& url, const std::function<const std::unique_ptr<FileReader>&(std::unique_ptr<FileReader>)>& registerReader, const std::function<const std::unique_ptr<FileWriter>&(std::unique_ptr<FileWriter>)>& registerWriter, const std::function<const std::unique_ptr<FileAppender>&(std::unique_ptr<FileAppender>)>& registerAppender);

		const Azure::Core::Url& GetUrl() const;
		bool HasDirUrl() const;
		void CheckUrl() const;
		virtual std::vector<std::string> UrlPathParts() const = 0;
		virtual void CheckFileUrl() const = 0;
		virtual void CheckDirUrl() const = 0;

		std::function<const std::unique_ptr<FileReader>& (std::unique_ptr<FileReader>)> RegisterReader;
		std::function<const std::unique_ptr<FileWriter>& (std::unique_ptr<FileWriter>)> RegisterWriter;
		std::function<const std::unique_ptr<FileAppender>& (std::unique_ptr<FileAppender>)> RegisterAppender;

	private:
		Azure::Core::Url url;
		bool bHasDirUrl;
	};
}
