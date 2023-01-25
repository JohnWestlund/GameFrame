# requires viu. run "p" in GameFrame art directory for cli preview
alias p='for I in $(ls *bmp|sort -n); do clear; viu -w 16 $I; [ -r config.ini ] && st=$(echo "$(grep "hold =" config.ini | sed "s#hold = ##" | tr -d $"\r")/1000"| bc -l) || st=0.1; sleep $st; done'
