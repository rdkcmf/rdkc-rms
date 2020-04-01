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

require_once 'responses/BadRMSResponse.php';
require_once 'responses/BaseRMSResponse.php';
require_once 'responses/ListPullPushConfigRMSResponse.php';
require_once 'responses/VersionRMSResponse.php';

class RServer {

	private $_ip;
	private $_port;

	/**
	 *
	 * @param string $ip the ip of the server
	 * @param string $port the port of the server
	 */
	public function __construct($ip, $port) {
		$this->_ip = $ip;
		$this->_port = $port;
	}

	private function call($resultType, $command, $params = "") {
		try {
			$fullCommand = "http://" . $this->_ip . ":" . $this->_port . "/" . $command;
			if (!($params === ""))
				$fullCommand.="?params=" . $params;
			$curl = curl_init($fullCommand);
			curl_setopt($curl, CURLOPT_RETURNTRANSFER, true);
			$httpResponse = curl_exec($curl);
			$res = new $resultType(json_decode($httpResponse));
			if (!$res->validate()) {
				return null;
			}
			return $res;
		} catch (Exception $ex) {
			return new ExceptionRMSResponse($ex);
		}
	}

	/**
	 * Tests the connection to the designated RDKC Media Server
	 * @return boolean true on success, otherwise returns false
	 */
	public function testServer() {
		$res = $this->version();
		if ($res == null)
			return false;
		return $res->status();
	}

	/**
	 * Instructs the server to create a new pull
	 * @param string $uri the source URI
	 * @param string $keepAlive reconnect on disconnect
	 * @param string $localStreamName the name of the new stream
	 * @return PullStreamRMSResponse
	 */
	public function pullStream($uri, $keepAlive, $localStreamName) {
		$uri = str_replace(" ", "\\ ", $uri);
		$localStreamName = str_replace(" ", "\\ ", $localStreamName);
		$keepAlive = strtoupper($keepAlive);
		if (($keepAlive === "ON")
				|| ($keepAlive === "1")
				|| ($keepAlive === "TRUE")
				|| ($keepAlive === "CHECKED")) {
			$keepAlive = 1;
		} else {
			$keepAlive = 0;
		}
		$params = "uri=" . $uri . " localStreamName=" . $localStreamName . " keepAlive=" . $keepAlive;
		$params = base64_encode($params);
		return $this->call("PullStreamRMSResponse", "pullStream", $params);
	}

	public function version() {
		return $this->call("VersionRMSResponse", "version");
	}

	public function help() {
		return $this->call("HelpRMSResponse", "help");
	}

	/**
	 * Obtains a list of the configured pull/push/hls/hds streams
	 * @return ListPullPushConfigRMSResponse|null The list of the configured streams
	 */
	public function listPullPushConfig() {
		return $this->call("ListPullPushConfigRMSResponse", "listPullPushConfig");
	}

}

?>
