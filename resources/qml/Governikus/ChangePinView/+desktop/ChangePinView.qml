/*
 * \copyright Copyright (c) 2015-2019 Governikus GmbH & Co. KG, Germany
 */

import QtQuick 2.10
import QtQuick.Controls 2.3

import Governikus.EnterPasswordView 1.0
import Governikus.Global 1.0
import Governikus.TitleBar 1.0
import Governikus.ProgressView 1.0
import Governikus.ResultView 1.0
import Governikus.SettingsView 1.0
import Governikus.View 1.0
import Governikus.Workflow 1.0
import Governikus.Type.SettingsModel 1.0
import Governikus.Type.ChangePinModel 1.0
import Governikus.Type.NumberModel 1.0


SectionPage {
	id: baseItem

	enum SubViews {
		Undefined,
		Workflow,
		Password,
		PasswordInfo,
		Progress,
		CardPosition,
		InputError,
		Data,
		PinUnlocked,
		Result,
		ReturnToMain,
		ReaderSettings
	}

	Keys.onEscapePressed: if (d.cancelAllowed) ChangePinModel.cancelWorkflow()

	titleBarAction: TitleBarAction {
		//: LABEL DESKTOP_QML
		text: qsTr("PIN Management") + SettingsModel.translationTrigger
		rootEnabled: false
		helpTopic: "pinTab"
		showSettings: (changePinController.workflowState === ChangePinController.WorkflowStates.Initial ||
		               changePinController.workflowState === ChangePinController.WorkflowStates.Reader ||
		               changePinController.workflowState === ChangePinController.WorkflowStates.Card)
		              && d.activeView !== ChangePinView.SubViews.Progress

		onClicked: {
			if (d.activeView === ChangePinView.SubViews.PasswordInfo) {
				d.view = ChangePinView.SubViews.Password
				ApplicationWindow.menuBar.updateActions()
			}
			else if (d.activeView === ChangePinView.SubViews.ReaderSettings) {
				d.view = readerView.preceedingView
				ApplicationWindow.menuBar.updateActions()
			}
		}

		customSubAction: CancelAction {
			visible: d.cancelAllowed

			onClicked: {
				if (pinResult.visible) {
					ChangePinModel.continueWorkflow()
					baseItem.nextView(SectionPage.Views.Main)
				} else {
					ChangePinModel.cancelWorkflow()
				}
			}
		}

		customSettingsHandler: function(){
			readerView.preceedingView = d.activeView
			d.view = ChangePinView.SubViews.ReaderSettings
			ApplicationWindow.menuBar.updateActions()
		}
	}

	QtObject {
		id: d

		property var view: ChangePinView.SubViews.Undefined
		readonly property int activeView: inputError.visible ? ChangePinView.SubViews.InputError : pinUnlocked.visible ? ChangePinView.SubViews.PinUnlocked : view
		readonly property bool cancelAllowed: ChangePinModel.isBasicReader || generalWorkflow.waitingFor != Workflow.WaitingFor.Password
	}

	TabbedReaderView {
		id: readerView

		property int preceedingView: ChangePinView.SubViews.Undefined

		visible: d.activeView === ChangePinView.SubViews.ReaderSettings
		onCloseView: {
			d.view = preceedingView
			ApplicationWindow.menuBar.updateActions()
		}
	}

	ChangePinController {
		id: changePinController

		onNextView: {
			if (pName === ChangePinView.SubViews.ReturnToMain) {
				baseItem.nextView(SectionPage.Views.Main)
				return;
			}

			d.view = pName
		}
	}

	GeneralWorkflow {
		id: generalWorkflow

		visible: d.activeView === ChangePinView.SubViews.Workflow

		isPinChange: true
		waitingFor: switch (changePinController.workflowState) {
						case ChangePinController.WorkflowStates.Reader:
							return Workflow.WaitingFor.Reader
						case ChangePinController.WorkflowStates.Card:
							return Workflow.WaitingFor.Card
						case ChangePinController.WorkflowStates.Password:
							return Workflow.WaitingFor.Password
						default:
							return Workflow.WaitingFor.None
		}
	}

	EnterPasswordView {
		id: enterPasswordView

		visible: d.activeView === ChangePinView.SubViews.Password

		onPasswordEntered: {
			d.view = ChangePinView.SubViews.Progress
			ChangePinModel.continueWorkflow()
		}

		onChangePinLength: {
			NumberModel.requestTransportPin = !NumberModel.requestTransportPin
		}

		onRequestPasswordInfo: {
			d.view = ChangePinView.SubViews.PasswordInfo
			ApplicationWindow.menuBar.updateActions()
		}
	}

	PasswordInfoView {
		id: passwordInfoView

		visible: d.activeView === ChangePinView.SubViews.PasswordInfo

		onClose: {
			d.view = ChangePinView.SubViews.Password
			ApplicationWindow.menuBar.updateActions()
		}
	}

	ProgressView {
		id: pinProgressView

		visible: d.activeView === ChangePinView.SubViews.Progress

		//: LABEL DESKTOP_QML
		text: qsTr("Change PIN") + SettingsModel.translationTrigger
		//: INFO DESKTOP_QML Processing screen text while the card communication is running after the PIN has been entered during PIN change process.
		subText: qsTr("Please wait a moment...") + SettingsModel.translationTrigger
	}

	ResultView {
		id: inputError

		property bool errorConfirmed: false

		visible: !errorConfirmed && NumberModel.hasPasswordError && d.view != ChangePinView.SubViews.Result

		resultType: ResultView.Type.IsError
		text: NumberModel.inputError
		onNextView: errorConfirmed = true

		Connections {
			target: NumberModel
			onFireInputErrorChanged: inputError.errorConfirmed = false
		}
	}

	ResultView {
		id: pinUnlocked

		property bool confirmed: true

		visible: !confirmed && (d.view === ChangePinView.SubViews.Password || generalWorkflow.waitingFor === Workflow.WaitingFor.Password)

		resultType: ResultView.Type.IsSuccess
		//: INFO DESKTOP_QML The ID card has just been unblocked and the user can now continue with their PIN change.
		text: qsTr("Your ID card is unblocked. You now have three more tries to change your PIN") + SettingsModel.translationTrigger
		onNextView: confirmed = true

		Connections {
			target: ChangePinModel
			onFireOnPinUnlocked: pinUnlocked.confirmed = false
		}
	}

	ResultView {
		id: cardPositionView

		visible: d.activeView === ChangePinView.SubViews.CardPosition

		resultType: ResultView.Type.IsInfo
		//: INFO DESKTOP_QML The NFC signal is weak or unstable, the user is asked to change the card's position to (hopefully) reduce the distance to the NFC chip.
		text: qsTr("Weak NFC signal.\nPlease reposition your card.") + SettingsModel.translationTrigger
		onNextView: ChangePinModel.continueWorkflow()
	}

	ResultView {
		id: pinResult

		visible: d.activeView === ChangePinView.SubViews.Result

		resultType: ChangePinModel.error ? ResultView.Type.IsError : ResultView.Type.IsSuccess
		text: ChangePinModel.resultString
		onNextView: {
			ChangePinModel.continueWorkflow()
			baseItem.nextView(pName)
		}
		emailButtonVisible: ChangePinModel.error && !ChangePinModel.isCancellationByUser()
		onEmailButtonPressed: ChangePinModel.sendResultMail()
	}
}
