/*!
 * \brief Worker implementation of ReaderManger thread
 *
 * \copyright Copyright (c) 2015-2019 Governikus GmbH & Co. KG, Germany
 */

#pragma once

#include "CardConnectionWorker.h"
#include "ReaderFilter.h"
#include "ReaderInfo.h"
#include "ReaderManagerPlugIn.h"
#include "ReaderManagerPlugInInfo.h"

#include <QObject>

namespace governikus
{
class ReaderManagerWorker
	: public QObject
{
	Q_OBJECT

	private:
		QVector<ReaderManagerPlugIn*> mPlugIns;

		void registerPlugIns();
		bool isPlugIn(const QJsonObject& pJson);
		void registerPlugIn(ReaderManagerPlugIn* pPlugIn);
		Reader* getReader(const QString& pReaderName) const;

	public:
		ReaderManagerWorker();
		~ReaderManagerWorker();

		Q_INVOKABLE void startScan(ReaderManagerPlugInType pType, bool pAutoConnect);
		Q_INVOKABLE void stopScan(ReaderManagerPlugInType pType, const QString& pError);
		Q_INVOKABLE bool isScanRunning() const;
		Q_INVOKABLE bool isScanRunning(ReaderManagerPlugInType pType) const;

		Q_INVOKABLE QVector<ReaderManagerPlugInInfo> getPlugInInfos() const;
		Q_INVOKABLE QVector<ReaderInfo> getReaderInfos(const ReaderFilter& pFilter = ReaderFilter()) const;
		Q_INVOKABLE ReaderInfo getReaderInfo(const QString& pReaderName) const;
		Q_INVOKABLE void updateReaderInfo(const QString& pReaderName);
		Q_INVOKABLE void createCardConnectionWorker(const QString& pReaderName);
		Q_INVOKABLE void updateRetryCounters();

	Q_SIGNALS:
		void firePluginAdded(const ReaderManagerPlugInInfo& pInfo);
		void fireStatusChanged(const ReaderManagerPlugInInfo& pInfo);
		void fireReaderAdded(const QString& pReaderName);
		void fireReaderRemoved(const QString& pReaderName);
		void fireReaderDeviceError(GlobalStatus::Code pError);
		void fireReaderPropertiesUpdated(const QString& pReaderName);
		void fireCardInserted(const QString& pReaderName);
		void fireCardRemoved(const QString& pReaderName);
		void fireCardRetryCounterChanged(const QString& pReaderName);
		void fireCardConnectionWorkerCreated(const QSharedPointer<CardConnectionWorker>& pCardConnectionWorker);
		void fireInitialized();

	public Q_SLOTS:
		void onThreadStarted();
};

} // namespace governikus
