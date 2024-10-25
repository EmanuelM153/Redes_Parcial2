dirs=$(ls -d */)

for dir in $dirs
do
	for i in {0..3}
	do
		file="${dir}hub-node${i}.pcap"
		jsonFile="${dir}hub-node${i}.json"
		tshark -r $file -T json -J "tcp" > $jsonFile &
	done
done
