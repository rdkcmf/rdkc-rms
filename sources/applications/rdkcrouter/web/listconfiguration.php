<?php
require_once 'rms/api/RServer.php';
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

$rms = new RServer("127.0.0.1", 7777);
if (!$rms->testServer()) {
	echo "Unable to connect to RDKC server";
	exit(0);
}

$res = $rms->listPullPushConfig();
if ($res == null) {
	echo "FAIL";
}
?>

<table border="1">
	<tr>
		<th>
			ID
		</th>
		<th>
			Local stream name
		</th>
		<th>
			URI
		</th>
		<th>
			Keep Alive
		</th>
		<th>
			Status
		</th>
	</tr>
	<?php
	foreach ($res->pulls() as /* @var $value PullConfig */ $value) {
		echo "<tr>";
		echo "<td>" . $value->id() . "</td>";
		echo "<td>" . $value->localStreamName() . "</td>";
		echo "<td>" . $value->uri() . "</td>";
		echo "<td>" . $value->keepAlive() . "</td>";
		echo "<td>" . $value->status()->description() . "</td>";
	}
	?>
</table>
