#include <QtPdf>
#include <QWidget>
#include <QPdfView>
#include <QProcess>
#include <QSettings>
#include <QFileInfo>
#include <QMimeDatabase>

#include <dlfcn.h>
#include "wlxplugin.h"

static char inipath[PATH_MAX];

HANDLE DCPCALL ListLoad(HANDLE ParentWin, char* FileToLoad, int ShowFlags)
{
	QProcess proc;
	QMimeDatabase db;
	QString cmd, key;

	QString filename = QString(FileToLoad);

	QFileInfo fi(filename);

	if (!fi.exists())
		return nullptr;

	QSettings settings(inipath, QSettings::IniFormat);
	settings.setIniCodec("UTF-8");

	key = QString("%1/command").arg(fi.suffix().toLower());

	if (settings.contains(key))
		cmd = QString::fromUtf8(settings.value(key).toByteArray());
	else
	{
		QMimeType type = db.mimeTypeForFile(filename);
		key = QString("%1/command").arg(type.name());

		if (settings.contains(key))
			cmd = QString::fromUtf8(settings.value(key).toByteArray());
	}

	if (cmd.isEmpty())
		return nullptr;

	QTemporaryDir tmpdir("/tmp/_dc-pdfview.XXXXXX");

	key.replace("/command", "/noautoremove");

	if (settings.value(key).toBool())
		tmpdir.setAutoRemove(false);

	QString out = QString("%1/output.pdf").arg(tmpdir.path());


	cmd.replace("$FILE", filename.replace(" ", "\\ "));
	cmd.replace("$PDF", out);

	QStringList params;
	params << "-c" << cmd;


	proc.start("/bin/sh", params);
	proc.waitForFinished();

	if (proc.exitStatus() != QProcess::NormalExit)
		return nullptr;

	QFileInfo fiout(out);

	if (!fiout.exists())
		return nullptr;

	QPdfDocument *document = new QPdfDocument();

	if (document->load(out) != QPdfDocument::NoError)
	{
		delete document;
		return NULL;
	}

	QPdfView *view = new QPdfView((QWidget*)ParentWin);
	view->setDocument(document);
	view->setPageMode(QPdfView::MultiPage);
	view->setZoomMode(QPdfView::FitToWidth);

	view->show();

	return view;
}

void DCPCALL ListCloseWindow(HANDLE ListWin)
{
	QPdfView *view = (QPdfView*)ListWin;
	QPdfDocument *document = view->document();

	if (document != NULL)
	{
		document->close();
		delete document;
	}

	delete view;
}

int DCPCALL ListSearchDialog(HWND ListWin, int FindNext)
{
	return LISTPLUGIN_OK;
}

void DCPCALL ListSetDefaultParams(ListDefaultParamStruct* dps)
{
	Dl_info dlinfo;
	memset(&dlinfo, 0, sizeof(dlinfo));

	if (dladdr(inipath, &dlinfo) != 0)
	{
		QFileInfo plugnfo(QString(dlinfo.dli_fname));
		snprintf(inipath, PATH_MAX, "%s/settings.ini", plugnfo.absolutePath().toStdString().c_str());
	}
}
