/*
 * \copyright Copyright (c) 2018-2019 Governikus GmbH & Co. KG, Germany
 */

import QtQuick 2.10

import Governikus.Global 1.0
import Governikus.Style 1.0
import Governikus.Type.ApplicationModel 1.0


Rectangle {
	id: baseItem

	property Item framee: parent
	property Item scope: parent
	property real marginFactor: 1
	property bool dynamic: true

	readonly property real staticSize: Math.max(ApplicationModel.scaleFactor * 4, 1)
	readonly property real dynamicSize: Math.max(Math.min(framee.width, framee.height) / 32, 1)
	readonly property real size: dynamic ? dynamicSize : staticSize

	anchors.fill: framee
	anchors.margins: marginFactor * -size * 2
	radius: size * 2
	border.width: scope.activeFocus ? size : 0;
	border.color: Constants.white
	opacity: 0.5
	color: Style.color.transparent
}
