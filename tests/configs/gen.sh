for i in {0..3} ; do ../build_ninja/src/keygen  > keys$i.txt ; mv key.pub key$i.pub  ; mv key.priv key$i.priv ; done 
