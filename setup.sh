if [ -z $1 ]
then
	echo "Missing new name"
	exit 1
fi

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

transform '@template@' `echo $1 | tr '[:upper:]' '[:lower:]'`
transform '@TEMPLATE@' `echo $1 | tr '[:lower:]' '[:upper:]'`
