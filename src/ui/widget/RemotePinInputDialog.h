/*!
 * \brief Dialog for PIN input for device pairing.
 *
 * \copyright Copyright (c) 2017-2019 Governikus GmbH & Co. KG, Germany
 */

#pragma once

#include <QDialog>

namespace Ui
{
class RemotePinInputDialog;
} // namespace Ui

namespace governikus
{

class RemotePinInputDialog
	: public QDialog
{
	Q_OBJECT

	private:
		QScopedPointer<Ui::RemotePinInputDialog> mUi;

	private Q_SLOTS:
		void onOkClicked();

	public:
		RemotePinInputDialog(QWidget* pParent = nullptr);
		virtual ~RemotePinInputDialog() override;

		static const QString getPin(QWidget* pParent);
		static bool isValidPin(const QString& pPin);
		QString getPinEntry() const;
};

} // namespace governikus
