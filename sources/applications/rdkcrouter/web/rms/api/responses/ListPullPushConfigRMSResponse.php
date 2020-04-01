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

require_once 'BaseRMSResponse.php';

class ConfigStatus {

	private $_raw;

	public function __construct($raw) {
		$this->_raw = $raw;
	}

	public function validate() {
		if (($this->_raw == null)
				|| (!(gettype($this->_raw->code) === "integer"))
				|| (!(gettype($this->_raw->description) === "string"))
				|| (!(gettype($this->_raw->timestamp) === "integer"))
		)
			return false;
		return true;
	}

	/**
	 * Returns the numeric code
	 * @return int
	 */
	public function code() {
		return $this->_raw->code;
	}

	/**
	 * Returns the human read-able description
	 * @return string
	 */
	public function description() {
		return $this->_raw->description;
	}

	/**
	 * Returns the unix timestamp for this status
	 * @return int
	 */
	public function timestamp() {
		return $this->_raw->timestamp;
	}

}

class PullConfig {

	private $_raw;

	public function __construct($raw) {
		$this->_raw = $raw;
	}

	public function validate() {
		if (($this->_raw == null)
				|| (!(gettype($this->_raw) === "object"))
				|| (!(gettype($this->_raw->configId) === "integer"))
				|| (!(gettype($this->_raw->emulateUserAgent) === "string"))
				|| (!(gettype($this->_raw->forceTcp) === "boolean"))
				|| (!(gettype($this->_raw->height) === "integer"))
				|| (!(gettype($this->_raw->isHds) === "boolean"))
				|| (!(gettype($this->_raw->isHls) === "boolean"))
				|| (!(gettype($this->_raw->keepAlive) === "boolean"))
				|| (!(gettype($this->_raw->localStreamName) === "string"))
				|| (!(gettype($this->_raw->pageUrl) === "string"))
				|| (!(gettype($this->_raw->rtcpDetectionInterval) === "integer"))
				|| (!(gettype($this->_raw->swfUrl) === "string"))
				|| (!(gettype($this->_raw->tcUrl) === "string"))
				|| (!(gettype($this->_raw->tos) === "integer"))
				|| (!(gettype($this->_raw->ttl) === "integer"))
				|| (!(gettype($this->_raw->uri) === "string"))
				|| (!(gettype($this->_raw->width) === "integer"))
				|| (!(gettype($this->_raw->status) === "object"))
				|| ($this->_raw->status == null)
				|| (!(gettype($this->_raw->status->current) === "object"))
				|| ($this->_raw->status->current == null)
		)
			return false;
		$temp = $this->_raw->status->current;
		$this->_raw->status->current = new ConfigStatus($temp);
		if (!$this->_raw->status->current->validate()) {
			return false;
		}
		return true;
	}

	/**
	 * Returns the unique ID of this configuration
	 * @return int
	 */
	public function id() {
		return $this->_raw->configId;
	}

	/**
	 * Returns the emulated user agent.
	 * Only meaningfull for URIs of type rtmp://...
	 * @return string
	 */
	public function emulateUserAgent() {
		return $this->_raw->emulateUserAgent;
	}

	/**
	 * Returns the forceTcp flag.
	 * Only meaningfull for URIs of type rtsp://...
	 * @return bool
	 */
	public function forceTcp() {
		return $this->_raw->forceTcp;
	}

	/**
	 * Returns the keepAlive flag.
	 * @return bool
	 */
	public function keepAlive() {
		return $this->_raw->keepAlive;
	}

	/**
	 * Returns the local stream name
	 * @return string
	 */
	public function localStreamName() {
		return $this->_raw->localStreamName;
	}

	/**
	 * Returns the pageUrl value used inside connect invoke
	 * Only meaningfull for URIs of type rtmp://...
	 * @return string
	 */
	public function pageUrl() {
		return $this->_raw->pageUrl;
	}

	/**
	 * Returns the rtcpDetectionInterval value
	 * Only meaningfull for URIs of type rtsp://...
	 * @return int
	 */
	public function rtcpDetectionInterval() {
		return $this->_raw->rtcpDetectionInterval;
	}

	/**
	 * Returns the swfUrl value used inside connect invoke
	 * Only meaningfull for URIs of type rtmp://...
	 * @return string
	 */
	public function swfUrl() {
		return $this->_raw->swfUrl;
	}

	/**
	 * Returns the tcUrl value used inside connect invoke
	 * Only meaningfull for URIs of type rtmp://...
	 * @return string
	 */
	public function tcUrl() {
		return $this->_raw->tcUrl;
	}

	/**
	 * Returns the TOS value assigned for the socket
	 * @return int
	 */
	public function tos() {
		return $this->_raw->tos;
	}

	/**
	 * Returns the TTL value assigned for the socket
	 * @return int
	 */
	public function ttl() {
		return $this->_raw->ttl;
	}

	/**
	 * Returns the URI
	 * @return string
	 */
	public function uri() {
		return $this->_raw->uri;
	}

	/**
	 * Returns the current status for the configuration
	 * @return ConfigStatus
	 */
	public function status() {
		return $this->_raw->status->current;
	}

}

class ListPullPushConfigRMSResponse extends BaseRMSResponse {

	public function validate() {
		if (!parent::validate())
			return false;
		if ((!(gettype($this->_object->data) === "object"))
				|| (!(gettype($this->_object->data->hls) === "array"))
				|| (!(gettype($this->_object->data->hds) === "array"))
				|| (!(gettype($this->_object->data->pull) === "array"))
				|| (!(gettype($this->_object->data->push) === "array"))
		)
			return false;
		if (!$this->validateHls())
			return false;
		if (!$this->validateHds())
			return false;
		if (!$this->validatePull())
			return false;
		if (!$this->validatePush())
			return false;
		return true;
	}

	private function validateHls() {
		return true;
	}

	private function validateHds() {
		return true;
	}

	private function validatePull() {
		$temp = $this->_object->data->pull;
		$this->_object->data->pull = array();
		foreach ($temp as $value) {
			$pullVal = new PullConfig($value);
			if (!$pullVal->validate()) {
				$this->_object->data->pull = null;
				return false;
			}
			array_push($this->_object->data->pull, $pullVal);
		}
		return true;
	}

	private function validatePush() {
		return true;
	}

	/**
	 * Returns an array containing all configurations for the pulled streams
	 * @return PullConfig[]|null
	 */
	public function pulls() {
		return $this->_object->data->pull;
	}
}

?>
