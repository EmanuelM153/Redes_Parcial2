dirs=$(ls -d */)

for dir in $dirs
do
	files=$(ls -1 ${dir}*pcap)
	for file in $files
	do
		newName=$(echo $file | sed 's/\.pcap$/\.json/')
		tshark -r $file -T json > $newName &
	done
done
