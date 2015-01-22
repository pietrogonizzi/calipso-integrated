#!/bin/bash 
# -xv for debug

#directories
scriptDir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
res="$scriptDir/res_analysis"

if [[ -d $res ]] ; then
      'rm' -rf $res
      'mkdir' $res
else
      'mkdir' $res
fi
rm -rf $res/index.html

echo "<html><body><h1>Results</h1><br>" > $res/index.html
#parameters
experimentationlist=(rpl)
#old topology
nodelist=(12638 28320 12652 12669 32357 32340)
#new topology
#nodelist=(12638 12681 32340 32357 12669 28320)

for e in ${experimentationlist[@]} ; do
	list="plot "
	echo "<h2>Experimentation: $e</h2>" >> $res/index.html
	for s in ${nodelist[@]} ; do
		echo "<h3>Results for node: $s</h3>" >> $res/index.html

		#Arrival time
		arr_time=$(cat results.$e | grep "_nodeinfo" |sed -e 's/\"/ /g' | sed -e 's/:/ /g' | sed -e 's/,/ /g' | sed -e 's/\}/ /g' |  sed -e 's/\[/ /g' | sed -e 's/\]/ /g'  | tr '\{' '\n' |grep "parking/"$s    | awk '{print $17}'| tail -1)
		
		
		#Energy instantaneous for each node over time
		cat results.$e | grep "energy" | sed -e 's/\// /g' | awk '($2=='"$s"') {print ($4-'"$arr_time"')/1000 " " $5}'  > $res/energy.$e.$s
		$scriptDir/plot.sh $res/enerygy.$e.$s $res/energy.$e.$s "time" "energy"
		echo "<center><img src='$res/enerygy.$e.$s.png'/></center>" >> $res/index.html

		#Consumed energy		
		cat results.$e | grep "energy" | sed -e 's/\// /g' | awk '($2=='"$s"') {print ($4-'"$arr_time"')/1000 " " $5*($4-'"$arr_time"')/1000 *3*0.02 }'  > $res/energy_int.$e.$s
		$scriptDir/plot.sh $res/enerygy_int.$e.$s $res/energy_int.$e.$s "time" "energy (mJ)"
		echo "<center><img src='$res/enerygy_int.$e.$s.png'/></center>" >> $res/index.html


		#PDR  for each node over time
		cat results.$e | grep "pdr" | sed -e 's/\"/ /g' | sed -e 's/:/ /g' | sed -e 's/,/ /g' | sed -e 's/\// /g' | sed -e 's/\}/ /g' | awk '($2=='"$s"'){print ($4-'"$arr_time"')/1000 " " $NF}' > $res/pdr.$e.$s #| sort -n -k 1 #> $res/pdr.$e.$s
		$scriptDir/plot.sh $res/pdr.$e.$s $res/pdr.$e.$s "time" "pdr"
		echo "<center><img src='$res/pdr.$e.$s.png'/></center>" >> $res/index.html		

		#Avg delay for each node over time
		cat results.$e | grep "delay" | sed -e 's/\// /g' | sed -e 's/\"/ /g' | sed -e 's/:/ /g' | sed -e 's/,/ /g' | awk '(($9==1)&&($11==1)){print $2 " " $4 " " $17}'  | awk '($1=='"$s"'){print ($2-'"$arr_time"')/1000 " " $3}'  > $res/delay.$e.$s
		$scriptDir/plot.sh $res/delay.$e.$s $res/delay.$e.$s "time" "avg delay (ms)"
		echo "<center><img src='$res/delay.$e.$s.png'/></center>" >> $res/index.html			

		#Overhead for each node over time
		cat results.$e | grep "overhead" | sed -e 's/\// /g' | awk '{print $2 " " $4 " " $5+$6+$7+$8+$9+$10}' | awk '($1=='"$s"'){print ($2-'"$arr_time"')/1000 " " $3}'  > $res/overhead.$e.$s
		$scriptDir/plot.sh $res/overhead.$e.$s $res/overhead.$e.$s "time" "overhead (packets)"
		echo "<center><img src='$res/overhead.$e.$s.png'/></center>" >> $res/index.html				
		
	done
done

echo "</body></html>" >> $res/index.html
