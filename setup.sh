transform () {
	files=$(grep -r --exclude-dir external --exclude-dir .git "$1" -l --exclude setup.sh)
	for f in $files 
	do
		$(sed -i s/$1/$2/ $f)
	done
	
	files=$(find -name "@template@*")
	for f in $files
	do
		mv $f $(echo $f | sed "s/$1/$2/")		
	done
}

transform 'bethutil-common' bethutil
transform 'BETHUTIL-COMMON' BETHUTIL

transform 'bethutil-bsa' bethutil
transform 'BETHUTIL-BSA' BETHUTIL

transform 'bethutil-tex' bethutil
transform 'BETHUTIL-TEX' BETHUTIL

transform 'bethutil-hkx' bethutil
transform 'BETHUTIL-HKX' BETHUTIL