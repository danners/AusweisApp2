/*!
 * \copyright Copyright (c) 2015-2019 Governikus GmbH & Co. KG, Germany
 */

#include "ReaderDeviceGui.h"

#include <QLoggingCategory>

using namespace governikus;

Q_DECLARE_LOGGING_CATEGORY(gui)

ReaderDeviceGui::ReaderDeviceGui(QWidget* pParentWidget)
	: QObject(pParentWidget)
	, mDialog(nullptr)
{
}


ReaderDeviceGui::~ReaderDeviceGui()
{
}


void ReaderDeviceGui::activate()
{
	if (mDialog == nullptr)
	{
		QWidget* dialogParent = qobject_cast<QWidget*>(parent());
		if (dialogParent == nullptr)
		{
			return;
		}

		mDialog = new ReaderDeviceDialog(dialogParent);
		connect(mDialog, &ReaderDeviceDialog::finished, this, &ReaderDeviceGui::onFinished);
		connect(mDialog, &QDialog::finished, this, &ReaderDeviceGui::fireFinished);
	}
	reactivate();
}


void ReaderDeviceGui::deactivate()
{
	if (mDialog != nullptr)
	{
		mDialog->close();
	}
}


void ReaderDeviceGui::reactToReaderCount(int pReaderCount)
{
	if (mDialog != nullptr && pReaderCount > 0)
	{
		mDialog->close();
	}
}


void ReaderDeviceGui::reactivate()
{
	if (mDialog->isMinimized())
	{
		mDialog->showNormal();
	}
	if (!mDialog->isVisible())
	{
		mDialog->show();
	}
	mDialog->activateWindow();
	mDialog->raise();
}


void ReaderDeviceGui::onFinished(int result)
{
	Q_UNUSED(result)

	mDialog = nullptr;
}
