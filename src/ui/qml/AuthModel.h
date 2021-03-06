/*!
 * \brief Model implementation for the authentication action.
 *
 * \copyright Copyright (c) 2015-2019 Governikus GmbH & Co. KG, Germany
 */

#pragma once

#include "context/AuthContext.h"
#include "Env.h"
#include "WorkflowModel.h"

#include <QObject>
#include <QQmlEngine>
#include <QSharedPointer>
#include <QString>

namespace governikus
{

class AuthModel
	: public WorkflowModel
{
	Q_OBJECT
	friend class Env;

	Q_PROPERTY(QString transactionInfo READ getTransactionInfo NOTIFY fireTransactionInfoChanged)
	Q_PROPERTY(int progressValue READ getProgressValue NOTIFY fireProgressChanged)
	Q_PROPERTY(QString progressMessage READ getProgressMessage NOTIFY fireProgressChanged)

	private:
		QSharedPointer<AuthContext> mContext;
		QString mTransactionInfo;

	protected:
		AuthModel();
		~AuthModel() override = default;
		static AuthModel& getInstance();

	public:
		void resetContext(const QSharedPointer<AuthContext>& pContext = QSharedPointer<AuthContext>());

		const QString& getTransactionInfo() const;
		int getProgressValue() const;
		const QString getProgressMessage() const;

		Q_INVOKABLE void setSkipRedirect(bool pSkipRedirect);

	private Q_SLOTS:
		void onDidAuthenticateEac1Changed();

	Q_SIGNALS:
		void fireTransactionInfoChanged();
		void fireProgressChanged();
};


} // namespace governikus
