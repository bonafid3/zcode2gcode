#ifndef ZipWriter_H
#define ZipWriter_H
#ifndef QT_NO_TEXTODFWRITER

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the ZipWriter class.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qstring.h>
#include <QtCore/qfile.h>

QT_BEGIN_NAMESPACE

class ZipWriterPrivate;


class ZipWriter
{
public:
	ZipWriter(const QString& fileName, QIODevice::OpenMode mode = (QIODevice::WriteOnly | QIODevice::Truncate));

	explicit ZipWriter(QIODevice* device);
	~ZipWriter();

	QIODevice* device() const;

	bool isWritable() const;
	bool exists() const;

	enum Status
	{
		NoError,
		FileWriteError,
		FileOpenError,
		FilePermissionsError,
		FileError
	};

	Status status() const;

	enum CompressionPolicy
	{
		AlwaysCompress,
		NeverCompress,
		AutoCompress
	};

	void setCompressionPolicy(CompressionPolicy policy);
	CompressionPolicy compressionPolicy() const;

	void setCreationPermissions(QFile::Permissions permissions);
	QFile::Permissions creationPermissions() const;

	void addFile(const QString& fileName, const QByteArray& data);

	void addFile(const QString& fileName, QIODevice* device);

	void addDirectory(const QString& dirName);

	void addSymLink(const QString& fileName, const QString& destination);

	void close();
private:
	ZipWriterPrivate* d;
	Q_DISABLE_COPY(ZipWriter)
};

QT_END_NAMESPACE

#endif // QT_NO_TEXTODFWRITER
#endif // ZipWriter_H
