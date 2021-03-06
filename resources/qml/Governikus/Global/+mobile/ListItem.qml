/*
 * \copyright Copyright (c) 2015-2019 Governikus GmbH & Co. KG, Germany
 */

import QtQuick 2.10
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.10

import Governikus.Global 1.0
import Governikus.Style 1.0
import Governikus.Type.SettingsModel 1.0

Rectangle {
	id: baseItem

	property alias icon: imageItem.source
	property alias headerText: headerItem.text
	property alias text: textItem.text
	property alias footerText: footerItem.text

	property real contentMarginLeft: Constants.groupbox_spacing
	property real contentMarginRight: Constants.groupbox_spacing

	property bool showRightArrow: Constants.is_layout_ios
	property bool showSeparator: true
	property bool pressed: mouseArea.pressed
	property alias mouseAreaEnabled: mouseArea.enabled

	signal clicked

	width: parent.width
	height: Style.dimens.list_item_height

	Accessible.role: Accessible.ListItem
	Accessible.name: headerText + ". " + text + ". " + footerText
	Accessible.onPressAction: if (Qt.platform.os === "ios") mouseArea.clicked(null)

	color: pressed ? Constants.lightgrey : Style.color.background_pane

	GSeparator {
		visible: showSeparator

		width: Constants.is_layout_ios ? (parent.width - textLayout.x - contentMarginLeft) : parent.width
		anchors.top: parent.bottom
		anchors.topMargin: -height
		anchors.right: parent.right
	}

	RowLayout {
		id: content

		anchors.fill: parent
		anchors.leftMargin: contentMarginLeft
		anchors.rightMargin: contentMarginRight

		spacing: Constants.groupbox_spacing

		Image {
			id: imageItem

			visible: baseItem.icon !== ""
			sourceSize.height: parent.height - 2 * Constants.groupbox_spacing

			asynchronous: true
			fillMode: Image.PreserveAspectFit
		}

		ColumnLayout {
			id: textLayout

			Layout.fillWidth: true

			spacing: 0

			GText {
				id: headerItem

				visible: baseItem.headerText !== ""
				Layout.fillWidth: true

				Accessible.ignored: true

				elide: Text.ElideRight
				textStyle: Style.text.hint_accent
				maximumLineCount: 1
			}

			GText {
				id: textItem

				visible: baseItem.text !== ""
				Layout.fillWidth: true

				Accessible.ignored: true

				elide: Text.ElideRight
				textStyle: Style.text.normal
				maximumLineCount: 2
			}

			GText {
				id: footerItem

				visible: baseItem.footerText !== ""
				Layout.fillWidth: true

				Accessible.ignored: true

				elide: Text.ElideRight
				textStyle: Style.text.hint_secondary
				maximumLineCount: 1
			}
		}

		Image {
			id: arrowRight

			visible: showRightArrow

			sourceSize.height: Style.dimens.small_icon_size
			fillMode: Image.PreserveAspectFit
			source: "qrc:///images/arrowRight.svg"

			ColorOverlay {
				anchors.fill: arrowRight
				source: arrowRight
				color: Style.color.secondary_text
			}
		}
	}

	MouseArea {
		id: mouseArea

		anchors.fill: parent

		onClicked: baseItem.clicked()
	}
}
