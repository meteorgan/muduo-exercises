cat $1 $2 $3 > data
sort -n data > sorted_data
uniq -c sorted_data | sort -k1,1nr -k2,2n > freq_data
