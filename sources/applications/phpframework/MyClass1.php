<?php
/**
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
##########################################################################
**/


require 'variant/CSerializableObject.php';

/**
 * Description of MyClass1
 *
 */
class MyClass1 extends CSerializableObject {
    public $fooString;
    public $barNumber;

    public function Serialize(&$rawArray) {
        $rawArray["fooString"]=$this->fooString;
        $rawArray["barNumber"]=$this->barNumber;
        return true;
    }
    public function Deserialize($rawArray) {
        if(!array_key_exists("fooString",$rawArray)) {
            return false;
        }
        if(!is_string($rawArray["fooString"])) {
            return false;
        }
        if(!array_key_exists("barNumber",$rawArray)) {
            return false;
        }
        if(!is_numeric($rawArray["barNumber"])) {
            return false;
        }
        $this->fooString=$rawArray["fooString"];
        $this->barNumber=intval($rawArray["barNumber"]);
        return true;
    }
}
?>
