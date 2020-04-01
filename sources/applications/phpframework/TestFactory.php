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


require 'variant/CClassFactory.php';
require 'MyClass1.php';
require 'MyClass2.php';
require 'MyClass3.php';

/**
 * Description of TestFactory
 *
 */
class TestFactory extends CClassFactory {
    protected function InstantiateObjectType($className) {
        if($className=="MyClass1") {
            return new MyClass1();
        } else if($className=="MyClass2") {
            return new MyClass2();
        } else if($className=="MyClass3") {
            return new MyClass3();
        } else {
            return null;
        }
    }
}
?>
