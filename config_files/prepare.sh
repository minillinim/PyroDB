# run these commands to split and parse the raw csv file
sed -i -e "s%microbial mat/biofilm%mat_biofilm%" MIMARKS_26_01_11_ENV.csv 
sed -i -e "s%miscellaneous natural or artificial environment%miscellaneous%" MIMARKS_26_01_11_ENV.csv
sed -i -e "s%human-%human_%" MIMARKS_26_01_11_ENV.csv
sed -i -e "s%host-%host_%" MIMARKS_26_01_11_ENV.csv
sed -i -e "s%wastewater/%wastewater_%" MIMARKS_26_01_11_ENV.csv
sed -i -e "s%plant-associated%plant_associated%" MIMARKS_26_01_11_ENV.csv

