/*!
 * \copyright Copyright (c) 2014-2019 Governikus GmbH & Co. KG, Germany
 */

#include "PcscCard.h"

#include "DestroyPaceChannel.h"
#include "EstablishPaceChannel.h"
#include "PinModify.h"

#include <QLatin1String>
#include <QLoggingCategory>
#include <QOperatingSystemVersion>

Q_DECLARE_LOGGING_CATEGORY(card_pcsc)

using namespace governikus;

namespace
{
QLatin1String protocolToString(PCSC_INT pProtocol)
{
	switch (pProtocol)
	{
		case SCARD_PROTOCOL_UNDEFINED:
			return QLatin1String("SCARD_PROTOCOL_UNDEFINED");

		case SCARD_PROTOCOL_T0:
			return QLatin1String("SCARD_PROTOCOL_T0");

		case SCARD_PROTOCOL_T1:
			return QLatin1String("SCARD_PROTOCOL_T1");

		case SCARD_PROTOCOL_RAW:
			return QLatin1String("SCARD_PROTOCOL_RAW");

#ifdef SCARD_PROTOCOL_T15
		case SCARD_PROTOCOL_T15:
			return QLatin1String("SCARD_PROTOCOL_T15");

#endif

		default:
			qCWarning(card_pcsc) << "Unknown value of SCARD_PROTOCOL:" << pProtocol;
			return QLatin1String();
	}
}


} // namespace


PcscCard::PcscCard(PcscReader* pPcscReader)
	: Card()
	, mReader(pPcscReader)
	, mProtocol(SCARD_PROTOCOL_UNDEFINED)
	, mContextHandle(0)
	, mCardHandle(0)
	, mTimer()
{
	PCSC_RETURNCODE returnCode = SCardEstablishContext(SCARD_SCOPE_USER, nullptr, nullptr, &mContextHandle);
	qCDebug(card_pcsc) << "SCardEstablishContext for" << mReader->getName() << ':' << PcscUtils::toString(returnCode);

	mTimer.setInterval(4000);
	QObject::connect(&mTimer, &QTimer::timeout, this, &PcscCard::sendSCardStatus);
}


PcscCard::~PcscCard()
{
	qCDebug(card_pcsc) << mReader->getName();
	if (PcscCard::isConnected())
	{
		PcscCard::disconnect();
	}

	PCSC_RETURNCODE returnCode = SCardReleaseContext(mContextHandle);
	qCDebug(card_pcsc) << "SCardReleaseContext for" << mReader->getName() << ':' << PcscUtils::toString(returnCode);
	mContextHandle = 0;
}


void PcscCard::sendSCardStatus()
{
	/*
	 * According to the documentation of SCardBeginTransaction:
	 *
	 * "If a transaction is held on the card for more than five seconds with no operations happening on that card,
	 * then the card is reset.
	 * Calling any of the Smart Card and Reader Access Functions or Direct Card Access Functions on the card that
	 * is transacted results in the timer being reset to continue allowing the transaction to be used."
	 *
	 * When sending a transmit request after resetting, we get a SCARD_W_RESET_CARD back. That breaks any active
	 * secure messaging channels and results in cancelled authentications.
	 *
	 * To work around that issue, we send a SCardStatus as ping every four seconds to prevent the timeout.
	 */
	SCardStatus(mCardHandle, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
}


CardReturnCode PcscCard::connect()
{
	if (PcscCard::isConnected())
	{
		qCCritical(card_pcsc) << "Card is already connected";
		return CardReturnCode::COMMAND_FAILED;
	}

	PCSC_INT shareMode = SCARD_SHARE_SHARED;
	PCSC_INT preferredProtocols = SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;

	PCSC_RETURNCODE returnCode = SCardConnect(mContextHandle, mReader->getState().szReader, shareMode, preferredProtocols, &mCardHandle, &mProtocol);
	qCDebug(card_pcsc) << "SCardConnect for" << mReader->getName() << ':' << PcscUtils::toString(returnCode) << "| cardHandle:" << mCardHandle << "| protocol:" << protocolToString(mProtocol);
	if (returnCode != PcscUtils::Scard_S_Success)
	{
		return CardReturnCode::COMMAND_FAILED;
	}

	returnCode = SCardBeginTransaction(mCardHandle);
	qCDebug(card_pcsc) << "SCardBeginTransaction for" << mReader->getName() << ':' << PcscUtils::toString(returnCode);
	if (returnCode != PcscUtils::Scard_S_Success)
	{
		SCardDisconnect(mCardHandle, SCARD_LEAVE_CARD);
		return CardReturnCode::COMMAND_FAILED;
	}

	if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows8)
	{
		mTimer.start();
	}
	return CardReturnCode::OK;
}


CardReturnCode PcscCard::disconnect()
{
	if (!PcscCard::isConnected())
	{
		qCCritical(card_pcsc) << "Card is already disconnected";
		return CardReturnCode::COMMAND_FAILED;
	}
	mTimer.stop();

	PCSC_RETURNCODE returnCode = SCardEndTransaction(mCardHandle, SCARD_LEAVE_CARD);
	qCDebug(card_pcsc) << "SCardEndTransaction for" << mReader->getName() << ':' << PcscUtils::toString(returnCode);

	returnCode = SCardDisconnect(mCardHandle, SCARD_RESET_CARD);
	mCardHandle = 0;
	mProtocol = SCARD_PROTOCOL_UNDEFINED;
	qCDebug(card_pcsc) << "SCardDisconnect for" << mReader->getName() << ':' << PcscUtils::toString(returnCode);

	return returnCode == PcscUtils::Scard_S_Success ? CardReturnCode::OK : CardReturnCode::COMMAND_FAILED;
}


bool PcscCard::isConnected()
{
	return mCardHandle != 0;
}


CardReturnCode PcscCard::transmit(const CommandApdu& pCmd, ResponseApdu& pRes)
{
	if (!isConnected())
	{
		qCCritical(card_pcsc) << "Card is not connected, abort transmit";
		return CardReturnCode::COMMAND_FAILED;
	}

	QByteArray receiveBuffer;
	PCSC_RETURNCODE returnCode = transmit(pCmd.getBuffer(), receiveBuffer);
	if (returnCode != PcscUtils::Scard_S_Success)
	{
		return CardReturnCode::COMMAND_FAILED;
	}

	ResponseApdu tempResponse;
	tempResponse.setBuffer(receiveBuffer);

	if (tempResponse.getSW1() == SW1::WRONG_LE_FIELD)
	{
		qCDebug(card_pcsc) << "got SW1 == 0x6c, retransmitting with new Le:" << tempResponse.getSW2();
		CommandApdu retransmitCommand(pCmd.getCLA(), pCmd.getINS(), pCmd.getP1(), pCmd.getP2(), pCmd.getData(), tempResponse.getSW2());
		returnCode = transmit(retransmitCommand.getBuffer(), receiveBuffer);
		if (returnCode != PcscUtils::Scard_S_Success)
		{
			return CardReturnCode::COMMAND_FAILED;
		}
	}

	while (tempResponse.getSW1() == SW1::MORE_DATA_AVAILABLE)
	{
		QByteArray tempReceiveBuffer;
		qCDebug(card_pcsc) << "got SW1 == 0x61, getting response with Le:" << tempResponse.getSW2();
		CommandApdu getResponseCommand(0, char(0xC0), 0, 0, QByteArray(), tempResponse.getSW2());
		returnCode = transmit(getResponseCommand.getBuffer(), tempReceiveBuffer);
		if (returnCode != PcscUtils::Scard_S_Success)
		{
			return CardReturnCode::COMMAND_FAILED;
		}

		tempResponse.setBuffer(tempReceiveBuffer);
		// cut off sw1 and sw2
		receiveBuffer.resize(receiveBuffer.size() - 2);
		receiveBuffer += tempReceiveBuffer;
	}

	pRes.setBuffer(receiveBuffer);
	return CardReturnCode::OK;
}


PCSC_RETURNCODE PcscCard::transmit(const QByteArray& pSendBuffer, QByteArray& pReceiveBuffer)
{
	const SCARD_IO_REQUEST* sendPci;
	switch (mProtocol)
	{
		case SCARD_PROTOCOL_T0:
			sendPci = SCARD_PCI_T0;
			break;

		case SCARD_PROTOCOL_T1:
			sendPci = SCARD_PCI_T1;
			break;

		case SCARD_PROTOCOL_RAW:
			sendPci = SCARD_PCI_RAW;
			break;

		default:
			qCDebug(card_pcsc) << "unsupported protocol";
			return PcscUtils::Scard_E_Proto_Mismatch;
	}

	auto [returnCode, buffer] = transmit(pSendBuffer, sendPci);
	pReceiveBuffer = buffer;

	/*
	 * Reconnecting makes only sense, when no secure messaging channel is active.
	 * Otherwise the secure messaging channel is destroyed and the transmit will fail anyway.
	 */
	if (returnCode == PcscUtils::Scard_W_Reset_Card && !CommandApdu::isSecureMessaging(pSendBuffer))
	{
		returnCode = SCardReconnect(mCardHandle, SCARD_SHARE_SHARED, mProtocol, SCARD_RESET_CARD, nullptr);
		qCDebug(card_pcsc) << "Reconnect to Card";

		if (returnCode != PcscUtils::Scard_S_Success)
		{
			qCCritical(card_pcsc) << "SCardReconnect failed:" << PcscUtils::toString(returnCode);
			return returnCode;
		}

		returnCode = SCardBeginTransaction(mCardHandle);
		qCDebug(card_pcsc) << "SCardBeginTransaction:" << PcscUtils::toString(returnCode);
		auto [retryReturnCode, retryBuffer] = transmit(pSendBuffer, sendPci);
		returnCode = retryReturnCode;
		pReceiveBuffer = retryBuffer;
	}

	return returnCode;
}


PcscCard::CardResult PcscCard::transmit(const QByteArray& pSendBuffer,
		const SCARD_IO_REQUEST* pSendPci)
{
	SCARD_IO_REQUEST recvPci;
	recvPci.dwProtocol = mProtocol;
	recvPci.cbPciLength = sizeof(SCARD_IO_REQUEST);

	QByteArray data(8192, '\0');
	auto dataReceived = static_cast<PCSC_INT>(data.size());

	qCDebug(card_pcsc) << "SCardTransmit cmdBuffer:" << pSendBuffer.toHex();
	const PCSC_RETURNCODE returnCode = SCardTransmit(mCardHandle,
			pSendPci,
			reinterpret_cast<PCSC_CUCHAR_PTR>(pSendBuffer.data()),
			static_cast<PCSC_INT>(pSendBuffer.size()),
			&recvPci,
			reinterpret_cast<PCSC_UCHAR_PTR>(data.data()),
			&dataReceived);

	qCDebug(card_pcsc) << "SCardTransmit for" << mReader->getName() << ':' << PcscUtils::toString(returnCode);

	if (dataReceived > INT_MAX)
	{
		qCCritical(card_pcsc) << "Max allowed receive buffer size exceeded";
		Q_ASSERT(dataReceived <= INT_MAX);
		return {PcscUtils::Scard_F_Unknown_Error};
	}

	data.resize(static_cast<int>(dataReceived));
	qCDebug(card_pcsc) << "SCardTransmit resBuffer:" << data.toHex();

	if (data.size() < 2)
	{
		qCCritical(card_pcsc) << "Response buffer smaller than 2";
		return {PcscUtils::Scard_F_Unknown_Error};
	}

	return {returnCode, data};
}


EstablishPaceChannelOutput PcscCard::establishPaceChannel(PacePasswordId pPasswordId,
		const QByteArray& pChat,
		const QByteArray& pCertificateDescription,
		quint8 pTimeoutSeconds)
{
	Q_UNUSED(pTimeoutSeconds)
	if (!mReader->hasFeature(FeatureID::EXECUTE_PACE))
	{
		return CardReturnCode::COMMAND_FAILED;
	}
	PCSC_INT cmdID = mReader->getFeatureValue(FeatureID::EXECUTE_PACE);

	EstablishPaceChannel builder;
	builder.setPasswordId(pPasswordId);
	builder.setChat(pChat);
	builder.setCertificateDescription(pCertificateDescription);

	auto [returnCode, controlRes] = control(cmdID, builder.createCommandData());
	if (returnCode != PcscUtils::Scard_S_Success)
	{
		qCWarning(card_pcsc) << "Control to establish PACE channel failed";
		return CardReturnCode::COMMAND_FAILED;
	}

	EstablishPaceChannelOutput output;
	output.parse(controlRes, pPasswordId);
	return output;
}


CardReturnCode PcscCard::destroyPaceChannel()
{
	if (!mReader->hasFeature(FeatureID::EXECUTE_PACE))
	{
		return CardReturnCode::COMMAND_FAILED;
	}
	PCSC_INT cmdID = mReader->getFeatureValue(FeatureID::EXECUTE_PACE);

	DestroyPaceChannelBuilder builder;
	auto [returnCode, controlRes] = control(cmdID, builder.createCommandData());
	if (returnCode != PcscUtils::Scard_S_Success)
	{
		qCWarning(card_pcsc) << "Control to destroy PACE channel failed";
		return CardReturnCode::COMMAND_FAILED;
	}
	return CardReturnCode::OK;
}


PcscCard::CardResult PcscCard::control(PCSC_INT pCntrCode, const QByteArray& pCntrInput)
{
	QByteArray buffer(2048, '\0');
	PCSC_INT len = 0;
	qCDebug(card_pcsc) << "SCardControl cmdBuffer:" << pCntrInput.toHex();
	PCSC_RETURNCODE returnCode = SCardControl(mCardHandle,
			pCntrCode,
			pCntrInput.constData(),
			static_cast<PCSC_INT>(pCntrInput.size()),
			buffer.data(),
			static_cast<PCSC_INT>(buffer.size()),
			&len);

	if (returnCode != PcscUtils::Scard_S_Success)
	{
		len = 0;
	}

	if (buffer.size() < static_cast<int>(len))
	{
		qCCritical(card_pcsc) << "Buffer size smaller than read length";
		Q_ASSERT(buffer.size() >= static_cast<int>(len));
		return {PcscUtils::Scard_F_Unknown_Error};
	}

	buffer.resize(static_cast<int>(len));
	qCDebug(card_pcsc) << "SCardControl for" << mReader->getName() << ':' << PcscUtils::toString(returnCode) << buffer.toHex();
	return {returnCode, buffer};
}


CardReturnCode PcscCard::setEidPin(quint8 pTimeoutSeconds, ResponseApdu& pResponseApdu)
{
	if (!mReader->hasFeature(FeatureID::MODIFY_PIN_DIRECT))
	{
		return CardReturnCode::COMMAND_FAILED;
	}
	PCSC_INT cmdID = mReader->getFeatureValue(FeatureID::MODIFY_PIN_DIRECT);

	PinModify pinModify(pTimeoutSeconds);
	auto [returnCode, controlRes] = control(cmdID, pinModify.createCcidForPcsc());
	if (returnCode != PcscUtils::Scard_S_Success)
	{
		qCWarning(card_pcsc) << "Modify PIN failed";
		return CardReturnCode::COMMAND_FAILED;
	}

	pResponseApdu.setBuffer(controlRes);
	return pResponseApdu.getCardReturnCode();
}
