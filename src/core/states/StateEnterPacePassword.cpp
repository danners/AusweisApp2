/*!
 * \copyright Copyright (c) 2018-2019 Governikus GmbH & Co. KG, Germany
 */

#include "StateEnterPacePassword.h"


using namespace governikus;


StateEnterPacePassword::StateEnterPacePassword(const QSharedPointer<WorkflowContext>& pContext)
	: AbstractState(pContext, false)
	, GenericContextContainer(pContext)
{
}


void StateEnterPacePassword::run()
{
	Q_EMIT fireContinue();
}
