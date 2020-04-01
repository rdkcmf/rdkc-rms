<?php
/*
##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2019 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#########################################################################

*/

require_once 'rms/api/RServer.php';

switch ($_POST["operation"]) {
	case "pull":
		doPull();
		break;
	case "push":
		doPush();
		break;
	case "createhls":
		doCreateHLS();
		break;
	default:
		echo "OOOOPS!!";
		exit();
		break;
}

function doPull() {
	$rms = new RServer("127.0.0.1", 7777);
	if (!$rms->testServer()) {
		echo "Unable to connect to server";
		exit(0);
	}

	//1. get the URI
	$uri = $_POST["uri"];
	$keepAlive = $_POST["keepAlive"];
	$localStreamName = $_POST["localStreamName"];
	$res = $rms->pullStream($uri, $keepAlive, $localStreamName);
	if ($res != null) {
		echo "SUCCESS\n";
	} else {
		echo "FAIL: " . $res->description() . "\n";
	}
}

function doPush() {

}

function doCreateHLS() {

}

?>
