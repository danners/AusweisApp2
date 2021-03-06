/*!
 * \brief Worker for \ref CardConnection that will do the job in \ref ReaderManagerWorker
 *
 * \copyright Copyright (c) 2014-2019 Governikus GmbH & Co. KG, Germany
 */

#pragma once

#include "asn1/SecurityInfos.h"
#include "CardReturnCode.h"
#include "CommandApdu.h"
#include "EstablishPaceChannel.h"
#include "FileRef.h"
#include "pace/SecureMessaging.h"
#include "Reader.h"
#include "ResponseApdu.h"
#include "SmartCardDefinitions.h"

#include <QByteArray>

namespace governikus
{

/*!
 * This class represents a connection to a smart card.
 */
class CardConnectionWorker
	: public QObject
	, public QEnableSharedFromThis<CardConnectionWorker>
{
	private:
		Q_OBJECT

		/*!
		 * The connection talks to the Card held by the Reader
		 */
		QPointer<Reader> mReader;

		/*!
		 * Object performing the cryptography needed by a secure messaging channel
		 */
		QScopedPointer<SecureMessaging> mSecureMessaging;

		inline QSharedPointer<const EFCardAccess> getEfCardAccess() const;

	private Q_SLOTS:
		void onReaderInfoChanged(const QString& pReaderName);

	protected:
		/*!
		 * The Card hold by the Reader is expected to be connected.
		 * The connection is closed, when the CardConnection is destroyed.
		 */
		explicit CardConnectionWorker(Reader* pReader);

		/*!
		 * Destroys the CardConnection and disconnects from the card.
		 */
		virtual ~CardConnectionWorker();

	public:
		static QSharedPointer<CardConnectionWorker> create(Reader* pReader);

		Q_INVOKABLE ReaderInfo getReaderInfo() const;

		void setPukInoperative();

		virtual CardReturnCode updateRetryCounter();

		virtual CardReturnCode readFile(const FileRef& pFileRef, QByteArray& pFileContent);

		virtual CardReturnCode transmit(const CommandApdu& pCommandApdu, ResponseApdu& pResponseApdu);

		/*!
		 * Performs PACE and establishes a PACE channel.
		 * If the Reader is a basic reader and the PACE channel is successfully established, the subsequent transmits will be secured using, secure messaging.
		 * I. e., a secure messaging channel is established.
		 */
		virtual EstablishPaceChannelOutput establishPaceChannel(PacePasswordId pPasswordId,
				const QString& pPasswordValue);

		/*!
		 * Performs PACE and establishes a PACE channel for later terminal authentication.
		 * If the Reader is a basic reader and the PACE channel is successfully established, the subsequent transmits will be secured using, secure messaging.
		 * I. e., a secure messaging channel is established.
		 */
		virtual EstablishPaceChannelOutput establishPaceChannel(PacePasswordId pPasswordId,
				const QString& pPasswordValue,
				const QByteArray& pChat,
				const QByteArray& pCertificateDescription);

		/*!
		 * Destroys a previously established PACE channel.
		 */
		virtual CardReturnCode destroyPaceChannel();

		/*!
		 * Sets the current workflow progress message. This is necessary for platforms like iOS,
		 * where interacting with a card leads to a dialog where the message needs to be updated.
		 */
		virtual void setProgressMessage(const QString& pMessage);

		/*!
		 * Destroys an established secure messaging channel, if there is one.
		 */
		Q_INVOKABLE virtual bool stopSecureMessaging();

		virtual CardReturnCode setEidPin(const QString& pNewPin, quint8 pTimeoutSeconds, ResponseApdu& pResponseApdu);

	Q_SIGNALS:
		void fireReaderInfoChanged(const ReaderInfo& pReaderInfo);
};

} // namespace governikus
