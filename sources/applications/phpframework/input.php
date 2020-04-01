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


require 'TestFactory.php';
require 'variant/CVariantSerializer.php';

$testInput1 = '
<MAP name="" isArray="false">
    <typed_map name="typed_map_instance1" isArray="false" typename="MyClass1">
        <STR name="fooString">bar</STR>
        <INT8 name="barNumber">123</INT8>
    </typed_map>
    <typed_map name="typed_map_instance2" isArray="false" typename="MyClass2">
        <STR name="barString">bar</STR>
        <INT8 name="fooNumber">123</INT8>
    </typed_map>
    <typed_map name="typed_map_instance3" isArray="false" typename="MyClass3">
        <STR name="fooString">bar</STR>
        <INT8 name="barNumber">123</INT8>
        <typed_map name="myClass2" isArray="false" typename="MyClass2">
            <STR name="barString">bar</STR>
            <INT8 name="fooNumber">123</INT8>
        </typed_map>
    </typed_map>
    <MAP name="testArray" isArray="false">
        <STR name="0x00000000">string 0</STR>
        <STR name="0x00000001">string 1</STR>
        <STR name="0x00000002">string 2</STR>
        <STR name="0x00000003">string 3</STR>
        <STR name="0x00000004">string 4</STR>
        <STR name="gigi">stiintza</STR>
    </MAP>
    <MAP name="header" isArray="false">
        <UINT32 name="channelId">3</UINT32>
        <UINT8 name="headerType">3</UINT8>
        <BOOL name="isAbsolute">false</BOOL>
        <UINT32 name="messageLength">242</UINT32>
        <UINT8 name="messageType">20</UINT8>
        <UINT32 name="streamId">0</UINT32>
        <UINT32 name="timestamp">0</UINT32>
    </MAP>
    <MAP name="invoke" isArray="false">
        <!-- INT64 name="int64_key">9223372036854780000</INT64 -->
                          <!--  9223372036854775807 -->
        <!-- UINT64 name="uint64_key">18446744073709600000</UINT64 -->
                          <!--  9223372036854775807 -->
        <STR name="functionName">connect</STR>
        <DOUBLE name="id">1.000</DOUBLE>
        <BOOL name="isFlex">false</BOOL>
        <MAP name="parameters" isArray="false">
            <MAP name="0x00000000" isArray="false">
                <STR name="app">vptests</STR>
                <DOUBLE name="audioCodecs">3191.000</DOUBLE>
                <DOUBLE name="capabilities">239.000</DOUBLE>
                <STR name="flashVer">MAC 10,1,51,66</STR>
                <BOOL name="fpad">false</BOOL>
                <DOUBLE name="objectEncoding">0.000</DOUBLE>
                <NULL name="pageUrl"></NULL>
                <NULL name="swfUrl"></NULL>
                <STR name="tcUrl">rtmp://localhost/vptests</STR>
                <DOUBLE name="videoCodecs">252.000</DOUBLE>
                <DOUBLE name="videoFunction">1.000</DOUBLE>
                <DATE name="date_key">2010-10-31</DATE>
                <TIME name="time_key">10:11:12.000</TIME>
                <TIMESTAMP name="timestamp_key">2010-10-31T10:11:12.000</TIMESTAMP>
            </MAP>
        </MAP>
    </MAP>
</MAP>
';

$testInput2 = '<?xml version="1.0" ?><MAP isArray="false" name=""><STR name="Buggy_node">&lt;map&gt;&amp;some other xml stuff&lt;/map&gt;</STR><MAP isArray="false" name="header"><UINT32 name="channelId">3</UINT32><UINT8 name="headerType">3</UINT8><BOOL name="isAbsolute">false</BOOL><UINT32 name="messageLength">242</UINT32><UINT8 name="messageType">20</UINT8><UINT32 name="streamId">0</UINT32><UINT32 name="timestamp">0</UINT32></MAP><MAP isArray="false" name="invoke"><STR name="functionName">connect</STR><DOUBLE name="id">1.000</DOUBLE><BOOL name="isFlex">false</BOOL><MAP isArray="false" name="parameters"><MAP isArray="false" name="0x00000000"><STR name="app">vptests</STR><DOUBLE name="audioCodecs">3191.000</DOUBLE><DOUBLE name="capabilities">239.000</DOUBLE><STR name="flashVer">MAC 10,1,51,66</STR><BOOL name="fpad">false</BOOL><DOUBLE name="objectEncoding">0.000</DOUBLE><NULL name="pageUrl" /><NULL name="swfUrl" /><STR name="tcUrl">rtmp://localhost/vptests</STR><DOUBLE name="videoCodecs">252.000</DOUBLE><DOUBLE name="videoFunction">1.000</DOUBLE></MAP></MAP></MAP></MAP>';

$classFactory = new TestFactory();
$variantSerializer = new CVariantSerializer();
$variantSerializer->classFactory = $classFactory;


if (!$variantSerializer->DeserializeFromXML($testInput1)) {
//if(!$variantSerializer->DeserializeFromXML($testInput2)) {
//if(!$variantSerializer->DeserializeFromXML(file_get_contents('php://input'))) {
	echo "DeserializeFromXML failed\n" . $variantSerializer->lastErrorDesc;
	exit;
}
//echo "DeserializeFromXML completed\n";
//var_dump($variantSerializer->deserializedObject);
//exit;


if (!$variantSerializer->SerializeToXML($variantSerializer->deserializedObject)) {
	echo "SerializeToXML failed\n" . $variantSerializer->lastErrorDesc;
	exit();
}
header("Content-Type: text/xml");
echo $variantSerializer->serializedXml;
?>
