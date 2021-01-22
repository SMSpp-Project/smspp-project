

#/bin/tcsh -l
 

# Canid - - - - - - - - - - - - - - - - - - - -

for f in ./../data/Canad/*.dat

do 
	fileName=${f}
	echo $fileName
        ./../MMCFtest ./test.mmcf $fileName s
	
done

# CanadN2 - - - - - - - - - - - - - - - - - - - -

for f in ./../data/CanadN2/*.dat

do 
	fileName=${f}
	echo $fileName
        ./../MMCFtest ./test.mmcf $fileName s
	
done

# Mnetgen - - - - - - - - - - - - - - - - - - - -

for f in ./../data/Mnetgen/*

do 
	fileName=${f%.*}
	echo $fileName
         ./../MMCFtest ./test.mmcf $fileName m
	
done

# JLF - - - - - - - - - - - - - - - - - - - -

for f in ./../data/JLF/ALK/*

do 
	fileName=${f%.*}
	echo $fileName
         ./../MMCFtest ./test.mmcf $fileName p
	
done

for f in ./../data/JLF/Assad/*

do 
	fileName=${f%.*}
	echo $fileName
         ./../MMCFtest ./test.mmcf $fileName p
	
done

for f in ./../data/JLF/Chen.DSP/*

do 
	fileName=${f%.*}
	echo $fileName
         ./../MMCFtest ./test.mmcf $fileName p
	
done

for f in ./../data/JLF/Chen.PSP/*

do 
	fileName=${f%.*}
	echo $fileName
         ./../MMCFtest ./test.mmcf $fileName p
	
done

for f in ./../data/JLF/Farvolden/*

do 
	fileName=${f%.*}
	echo $fileName
         ./../MMCFtest ./test.mmcf $fileName p
	
done

for f in ./../data/JLF/Powell/*

do 
	fileName=${f%.*}
	echo $fileName
         ./../MMCFtest ./test.mmcf $fileName p
	
done

# ----------- E N D  File - - - - - - - - - - - -