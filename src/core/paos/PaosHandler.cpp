/*!
 * \copyright Copyright (c) 2014-2018 Governikus GmbH & Co. KG, Germany
 */

#include "paos/retrieve/DidList.h"
#include "paos/retrieve/Disconnect.h"
#include "paos/retrieve/InitializeFramework.h"
#include "paos/retrieve/StartPaosResponse.h"
#include "PaosHandler.h"
#include "retrieve/DidAuthenticateEac1Parser.h"
#include "retrieve/DidAuthenticateEac2Parser.h"
#include "retrieve/DidAuthenticateEacAdditionalParser.h"
#include "retrieve/TransmitParser.h"

using namespace governikus;


PaosHandler::PaosHandler(const QByteArray& pXmlData)
	: ElementDetector(pXmlData)
	, mDetectedType(PaosType::UNKNOWN)
	, mParsedObject()
{
	detect();
	parse();
}


void PaosHandler::parse()
{
	if (mDetectedType == PaosType::INITIALIZE_FRAMEWORK)
	{
		setParsedObject(new InitializeFramework(mXmlData));
	}
	else if (mDetectedType == PaosType::DID_LIST)
	{
		setParsedObject(new DIDList(mXmlData));
	}
	else if (mDetectedType == PaosType::DID_AUTHENTICATE_EAC1)
	{
		setParsedObject(DidAuthenticateEac1Parser().parse(mXmlData));
	}
	else if (mDetectedType == PaosType::DID_AUTHENTICATE_EAC2)
	{
		setParsedObject(DidAuthenticateEac2Parser().parse(mXmlData));
	}
	else if (mDetectedType == PaosType::DID_AUTHENTICATE_EAC_ADDITIONAL_INPUT_TYPE)
	{
		setParsedObject(DidAuthenticateEacAdditionalParser().parse(mXmlData));
	}
	else if (mDetectedType == PaosType::TRANSMIT)
	{
		setParsedObject(TransmitParser().parse(mXmlData));
	}
	else if (mDetectedType == PaosType::DISCONNECT)
	{
		setParsedObject(new Disconnect(mXmlData));
	}
	else if (mDetectedType == PaosType::STARTPAOS_RESPONSE)
	{
		setParsedObject(new StartPaosResponse(mXmlData));
	}

}


void PaosHandler::setParsedObject(PaosMessage* pParsedObject)
{
	if (pParsedObject == nullptr)
	{
		qCCritical(paos) << "Error parsing message. This is not a valid" << mDetectedType;
		mDetectedType = PaosType::UNKNOWN;
	}
	else
	{
		mParsedObject = QSharedPointer<PaosMessage>(pParsedObject);
	}
}


bool PaosHandler::handleFoundElement(const QString& pElementName, const QString&, const QXmlStreamAttributes& pAttributes)
{
	if (pElementName == QLatin1String("InitializeFramework"))
	{
		mDetectedType = PaosType::INITIALIZE_FRAMEWORK;
	}
	else if (pElementName == QLatin1String("DIDList"))
	{
		mDetectedType = PaosType::DID_LIST;
	}
	else if (pElementName == QLatin1String("DIDAuthenticate"))
	{
		QStringList expectedElements;
		expectedElements.push_back(QStringLiteral("AuthenticationProtocolData"));
		detectStartElements(expectedElements);
	}
	else if (pElementName == QLatin1String("AuthenticationProtocolData"))
	{
		for (const auto& attribute : pAttributes)
		{
			if (attribute.value().endsWith(QLatin1String("EAC1InputType")))
			{
				mDetectedType = PaosType::DID_AUTHENTICATE_EAC1;
			}
			else if (attribute.value().endsWith(QLatin1String("EAC2InputType")))
			{
				mDetectedType = PaosType::DID_AUTHENTICATE_EAC2;
			}
			else if (attribute.value().endsWith(QLatin1String("EACAdditionalInputType")))
			{
				mDetectedType = PaosType::DID_AUTHENTICATE_EAC_ADDITIONAL_INPUT_TYPE;
			}
		}
	}
	else if (pElementName == QLatin1String("Transmit"))
	{
		mDetectedType = PaosType::TRANSMIT;
	}
	else if (pElementName == QLatin1String("Disconnect"))
	{
		mDetectedType = PaosType::DISCONNECT;
	}
	else if (pElementName == QLatin1String("StartPAOSResponse"))
	{
		mDetectedType = PaosType::STARTPAOS_RESPONSE;
	}
	return false;
}


void PaosHandler::detect()
{
	QStringList expectedElements;
	expectedElements.push_back(QStringLiteral("InitializeFramework"));
	expectedElements.push_back(QStringLiteral("DIDList"));
	expectedElements.push_back(QStringLiteral("DIDAuthenticate"));
	expectedElements.push_back(QStringLiteral("Transmit"));
	expectedElements.push_back(QStringLiteral("Disconnect"));
	expectedElements.push_back(QStringLiteral("StartPAOSResponse"));
	detectStartElements(expectedElements);
}


PaosType PaosHandler::getDetectedPaosType() const
{
	return mDetectedType;
}


QSharedPointer<PaosMessage> PaosHandler::getPaosMessage() const
{
	return mParsedObject;
}
