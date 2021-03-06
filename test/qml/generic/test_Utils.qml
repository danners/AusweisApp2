/*
 * \copyright Copyright (c) 2016-2019 Governikus GmbH & Co. KG, Germany
 */

import QtTest 1.10

import Governikus.Global 1.0

TestCase {
	name: "MathTests"


	function test_escapeHtml() {
		compare(Utils.escapeHtml("a&b"), "a&amp;b", "escape &")
		compare(Utils.escapeHtml("<br/>"), "&lt;br/&gt;", "escape < and >")
		compare(Utils.escapeHtml("\"Hello\""), "&quot;Hello&quot;", "escape \"")
	}
}
