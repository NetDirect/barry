#!/bin/bash


SDK=/opt/RIM/sdk
PREVERIFY=/opt/RIM/WTK2.5.2/bin


PATH=$PATH:$PREVERIFY java -jar $SDK/bin/rapc.jar \
	import=$SDK/lib/net_rim_api.jar \
	codename=$1 \
	$1.rapc \
	*.java 

