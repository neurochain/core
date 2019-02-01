# Rest API for Bot

## Quick Overview

* The bot doesn't save any private keys
* The REST API doesn't send or receive any private key
* The output and input are in JSON format


## API

* List of transactions ...[ok]
  - input : public key Array( x,y)
  - output : Array( Transaction) 
* Build raw hash for signature 
  - input : Array( Transactions ID)
  - output : Array ( Raw Transactions ( string ) )
* Send Transaction 
  - input : Array( Raw transactions), Signature 
  - ouput : bool( status), error( int ) 

* For Test : add coin base transaction
  - input : public key
  - output : int ( max ncc )