# Rest API
## Block
### Get block by id
**URL**: `/block/id`

**method**: `POST`

return the block with the corresponding id

**data**:
```
{
    type: SHA256,
    data: <block_id>
}
```

**example**
```
$ curl localhost:8080/block/id -d '{"type":"SHA256", "data":"ehLcF3IgQAOf2LqQtcIKnYsjdF4JQ5SgANSiHiqc3J4="}'
{"header":{"id":{"type":"SHA256","data":"ehLcF3IgQAOf2LqQtcIKnYsjdF4JQ5SgANSiHiqc3J4="},"timestamp":{"data":1562759886},"previousBlockHash":{"type":"SHA256","data":"QCUzO1SN+I+rkwn5VGv3npeH+JVqxLTqdo0u+W/jEHY="},"author":{"signature":{"type":"SHA256","data":"Q9/XCc/ka5AuUJ1wLJUvIkQjbqm/RyEtCN3TeMfwKEnWBu+mdWSV5XzK2gsGdIa6EDvpkPd9A6x2XVg8hPVf7Q=="},"keyPub":{"type":"ECP256K1","rawData":"Anjy3xeuFab7G1v7A8HBPPaTO/oYqVGdb0nOJevecP/h"}},"height":161616},"transactions":[{"id":{"type":"SHA256","data":"Z1+uk6pCAhahvafiiUvzNL8giag/Yw5VJUayXb+DMYc="},"inputs":[{"id":{"type":"SHA256","data":"T1u9AcC1e/tJfdP0laqAlzPZ+MB94/3KOLx5uNG3D+g="},"outputId":1,"signatureId":0},{"id":{"type":"SHA256","data":"7Ku6xw/kxD5aalMfm/T0OGC0t5ki2LyLO09NUhboAeE="},"outputId":0,"signatureId":0}],"outputs":[{"address":{"data":"N7kmiqPSAtzgzMLRFD6GbqamhdYXhq4ACD"},"value":{"value":"50"}},{"address":{"data":"NBjqvjJrgyh2M3BtQL1nBtsGf9ky384Eud"},"value":{"value":"51"}}],"signatures":[{"signature":{"type":"SHA256","data":"GjXQe9mfBUMON7Ams+tdTPjY2kD6jZt2siwtwucw02P1pYiG6jjEknI8zm72Ks6AG53EIQg0N/Rks1VGsQn8gw=="},"keyPub":{"type":"ECP256K1","rawData":"Anjy3xeuFab7G1v7A8HBPPaTO/oYqVGdb0nOJevecP/h"}},{"signature":{"type":"SHA256","data":"iL1fQZhODFVln4mZuN0Kh6cuDn1Ne28N7K6AdMWjW9yNug48ZeP7fnTxwhbRcAvUtsdCXUBHXN9zaISzn7Pxhg=="},"keyPub":{"type":"ECP256K1","rawData":"Anjy3xeuFab7G1v7A8HBPPaTO/oYqVGdb0nOJevecP/h"}}]}],"coinbase":{"id":{"type":"SHA256","data":"WJygVld/K9I4Nic71/v1rcKx2V+IGuQOCAEWLH6uY5I="},"outputs":[{"address":{"data":"NBjqvjJrgyh2M3BtQL1nBtsGf9ky384Eud"},"value":{"value":"100"},"data":""}],"coinbaseHeight":161616}}
```

### Get block by height
**URL**: `/block/height/:height`

**URL Parameter**: `height` height of the block you want info on

**method**: `GET`

return the block corresponding to a certain height

**example**:
```
$ curl localhost:8080/block/height/0
{"header":{"id":{"type":"SHA256","data":"he8knu6idJCQEADAPD1vkmo8OzIBBJ58Bb3apnB9+sE="},"timestamp":{"data":1560335646},"previousBlockHash":{"type":"SHA256","data":""},"author":{"signature":{"type":"SHA256","data":"Dg5gfxNN0teVG3auB72eEcH89LHA/YcuIxSB42INO5r7C66JuhfLXhIe2IHIHOUpC5X6M6yfHQwTycgUNIoqbw=="},"keyPub":{"type":"ECP256K1","rawData":"At2zvnKjc8FhInceEx4LWJ3Yy6DyD83MR3jlCneoZZpP"}},"height":0},"coinbase":{"id":{"type":"SHA256","data":"QVu0VZ6AS5XmGA/f0rkGxKkg1XO4F2DJ3fuZF+4CwJU="},"outputs":[{"address":{"data":"NEcd7W2csRbQJz7NeTNGHGoNoGvrw7EkvL"},"value":{"value":"1152921504606846976"},"data":""},{"address":{"data":"N9i6KhU5mUPrS3uTQCtwTNeEztRM318Cik"},"value":{"value":"1152921504606846976"},"data":""}],"coinbaseHeight":0}}
```


### Get latest block
**URL**: `/last_blocks/:number`

**URL Parameter**: `number` quantity of block you want

**method**: `GET`

get the last N block

**example**:
```
$ curl http://52.47.129.155:8080/last_blocks/2
{"block":[{"header":{"id":{"type":"SHA256","data":"3pNSyJPlhBMPzvV8fqCot/0obdTaFI+EJqddWd3zuZ4="},"timestamp":{"data":1562621676},"previousBlockHash":{"type":"SHA256","data":"fZPA9sA2vZWKDAZNY6L5uG/9042xzrIJxk5798WRK4I="},"author":{"signature":{"type":"SHA256","data":"fv8PxaN4MpZ3B3MZHa5KVUnAuGuwNHoUmfIA06vRwQMCjEuOqgqY2wY2Pfzy/eKK8nktaRryJf+sAeKqqMeWig=="},"keyPub":{"type":"ECP256K1","rawData":"Am4regJZ+i3Z7QVhreQ05qVKxis00ib2l98I/T+dx/ZE"}},"height":152402},"transactions":[{"id":{"type":"SHA256","data":"22GsjS8+hNaB/7G3X6WGQEFYKa2KyfkhcrjTh6qrKFw="},"inputs":[{"id":{"type":"SHA256","data":"79B1e2exKBbJXc+QHiVqRPPtO8tfmEfxYSzk/QzSCcQ="},"outputId":1,"signatureId":0}],"outputs":[{"address":{"data":"N9i6KhU5mUPrS3uTQCtwTNeEztRM318Cik"},"value":{"value":"25"}},{"address":{"data":"NH5Sr4NDcLDdEd5Gc6byrA2Rxh6ndR5E2N"},"value":{"value":"26"}}],"signatures":[{"signature":{"type":"SHA256","data":"yspA2OUaUIp1NFjxd881IkuQDKVtdG4E2pv7S+5UI/41GBh+f9shBpcsJ6ufHZSxsmb7ZX1NJ5YOODHwTpgGqA=="},"keyPub":{"type":"ECP256K1","rawData":"Am4regJZ+i3Z7QVhreQ05qVKxis00ib2l98I/T+dx/ZE"}}]},{"id":{"type":"SHA256","data":"79B1e2exKBbJXc+QHiVqRPPtO8tfmEfxYSzk/QzSCcQ="},"inputs":[{"id":{"type":"SHA256","data":"wSdT7oEUxvkjqrc6vfCbij6AfirHvGebfVd4LmN8O6Y="},"outputId":0,"signatureId":0},{"id":{"type":"SHA256","data":"MLAOopyP33JBc1bZC8k3rFFod4jGmgEgpFJVYfBH6is="},"outputId":1,"signatureId":0}],"outputs":[{"address":{"data":"N7BiWVGenpHu7w7rd8vF1pmxhtnMEYE45H"},"value":{"value":"50"}},{"address":{"data":"NH5Sr4NDcLDdEd5Gc6byrA2Rxh6ndR5E2N"},"value":{"value":"51"}}],"signatures":[{"signature":{"type":"SHA256","data":"DBucSVeI4Mch/Lsvn3y2TPHgw0hdWLtLacD5YTOh+YRPWFPPAEA3EXKUSjd+HRerd7QQ5+sO0cxTqNpoe+OweQ=="},"keyPub":{"type":"ECP256K1","rawData":"Am4regJZ+i3Z7QVhreQ05qVKxis00ib2l98I/T+dx/ZE"}},{"signature":{"type":"SHA256","data":"3Ff9EKflDBynfAw178hDrQvN+f6wT5ovsrXLQ0hAn3MMGvT3CpeKrhZMMbNxiUs8eEjm7axmYGUJ6XqY7F446A=="},"keyPub":{"type":"ECP256K1","rawData":"Am4regJZ+i3Z7QVhreQ05qVKxis00ib2l98I/T+dx/ZE"}}]},{"id":{"type":"SHA256","data":"CBsJv2T6ydW42/tGtdz2lwJIqBa7LiprRkla2pGpT4Y="},"inputs":[{"id":{"type":"SHA256","data":"22GsjS8+hNaB/7G3X6WGQEFYKa2KyfkhcrjTh6qrKFw="},"outputId":1,"signatureId":0}],"outputs":[{"address":{"data":"N7BiWVGenpHu7w7rd8vF1pmxhtnMEYE45H"},"value":{"value":"13"}},{"address":{"data":"NH5Sr4NDcLDdEd5Gc6byrA2Rxh6ndR5E2N"},"value":{"value":"13"}}],"signatures":[{"signature":{"type":"SHA256","data":"64j+oxl8a3BXl8VJqDHQM4Jr2ecoxXm8scS4j6iEDcNYAMQVW9KnqTtVHtWtuBiX+kAYNSG2KTnW3A5RMhfDZg=="},"keyPub":{"type":"ECP256K1","rawData":"Am4regJZ+i3Z7QVhreQ05qVKxis00ib2l98I/T+dx/ZE"}}]},{"id":{"type":"SHA256","data":"MLAOopyP33JBc1bZC8k3rFFod4jGmgEgpFJVYfBH6is="},"inputs":[{"id":{"type":"SHA256","data":"VhstOOA/cjoZnayXN8n16W3A2Itm59H+o+aVzZuaH3s="},"outputId":1,"signatureId":0}],"outputs":[{"address":{"data":"NEcd7W2csRbQJz7NeTNGHGoNoGvrw7EkvL"},"value":{"value":"1"}},{"address":{"data":"NH5Sr4NDcLDdEd5Gc6byrA2Rxh6ndR5E2N"},"value":{"value":"1"}}],"signatures":[{"signature":{"type":"SHA256","data":"hx4qpPaxXNuFqpWLgjZH7qFkHXa2XJeH0C4jG8xshzBUdu1djB9zSTVX1wAPiMsleH5tQLZ4HGE9ctY7Gpo2ug=="},"keyPub":{"type":"ECP256K1","rawData":"Am4regJZ+i3Z7QVhreQ05qVKxis00ib2l98I/T+dx/ZE"}}]}],"coinbase":{"id":{"type":"SHA256","data":"4CxYEe+AsOENLRqVGTxb4phtxchbfjNGY/DtKd/KT10="},"outputs":[{"address":{"data":"NH5Sr4NDcLDdEd5Gc6byrA2Rxh6ndR5E2N"},"value":{"value":"100"},"data":""}],"coinbaseHeight":152402}},{"header":{"id":{"type":"SHA256","data":"fZPA9sA2vZWKDAZNY6L5uG/9042xzrIJxk5798WRK4I="},"timestamp":{"data":1562621496},"previousBlockHash":{"type":"SHA256","data":"hgBBGkI/cLujO9B4z+Ur//ZNpi9eRo/Ol5kVGhLSejo="},"author":{"signature":{"type":"SHA256","data":"RTBg5yp/FhXkbsx+AzgbVpC8XyRv/cBXZhKRm3tyjduTaeUh3Oqon1Kcf3BsF3oQAYOJu8pQagD3bEiGrNpySw=="},"keyPub":{"type":"ECP256K1","rawData":"Am4regJZ+i3Z7QVhreQ05qVKxis00ib2l98I/T+dx/ZE"}},"height":152390},"transactions":[{"id":{"type":"SHA256","data":"77FJSI0TN2PKG1sSMzdxXulTgYksFnmpAhcaeQv2gT0="},"inputs":[{"id":{"type":"SHA256","data":"lOGg7isE2tJGMVHjXMz2bwlNhrewHr4Wk/jo278jNt4="},"outputId":1,"signatureId":0}],"outputs":[{"address":{"data":"N9i6KhU5mUPrS3uTQCtwTNeEztRM318Cik"},"value":{"value":"3"}},{"address":{"data":"NH5Sr4NDcLDdEd5Gc6byrA2Rxh6ndR5E2N"},"value":{"value":"4"}}],"signatures":[{"signature":{"type":"SHA256","data":"yMnsiogumdolz/20oNtyR2O/lF/sTCA4AvoFzofKJ8kMe1lwcLW8CHVDPqgvMfUdWhJ7HlKSKkG/j2wbFgJ4HA=="},"keyPub":{"type":"ECP256K1","rawData":"Am4regJZ+i3Z7QVhreQ05qVKxis00ib2l98I/T+dx/ZE"}}]},{"id":{"type":"SHA256","data":"GTgLuFXzp8IxjChAdaIPdTUZrznPwDvQT6uwM7qmi8Y="},"inputs":[{"id":{"type":"SHA256","data":"zgFWx0/N0rX5CmxAEBofWwHPgXNz3O3n/JQgMUdLz4k="},"outputId":0,"signatureId":0},{"id":{"type":"SHA256","data":"h4PKFa0Iu9+mCPnJyOO2uYQiHJoGUk2CKSUB/mqe/DU="},"outputId":1,"signatureId":0}],"outputs":[{"address":{"data":"N7BiWVGenpHu7w7rd8vF1pmxhtnMEYE45H"},"value":{"value":"50"}},{"address":{"data":"NH5Sr4NDcLDdEd5Gc6byrA2Rxh6ndR5E2N"},"value":{"value":"51"}}],"signatures":[{"signature":{"type":"SHA256","data":"zi1b9QXV/j5iDeQf0VcbtPsdZepvJUuAV/o+12j+eQgsMPkV+LYz4IPDP2/5nrOJUjYefNtpEstd1SPQqKtHzg=="},"keyPub":{"type":"ECP256K1","rawData":"Am4regJZ+i3Z7QVhreQ05qVKxis00ib2l98I/T+dx/ZE"}},{"signature":{"type":"SHA256","data":"lwgrAsz6aD+ma7hUPR8L++1VlJTTeZ5aZbaDU0LKYrs6vPMzoVMRLHp8PBOFjxfJH8F9D8ZfoKe8PjNTyIINKg=="},"keyPub":{"type":"ECP256K1","rawData":"Am4regJZ+i3Z7QVhreQ05qVKxis00ib2l98I/T+dx/ZE"}}]},{"id":{"type":"SHA256","data":"MEidA+ycYRTqx2NeYFEaGBNohYQLoffbrud4ZBJG3xY="},"inputs":[{"id":{"type":"SHA256","data":"osN9+bFKntO2KAVi6XjhLnJwnyUzOJCdDkp4xPMqezY="},"outputId":1,"signatureId":0}],"outputs":[{"address":{"data":"NEcd7W2csRbQJz7NeTNGHGoNoGvrw7EkvL"},"value":{"value":"13"}},{"address":{"data":"NH5Sr4NDcLDdEd5Gc6byrA2Rxh6ndR5E2N"},"value":{"value":"13"}}],"signatures":[{"signature":{"type":"SHA256","data":"wqocBceVmAyI20m4+vTiWgzMiC2T1aRyTX07kIIJ7Ji0sN5/rAjwMpYmxP4tDziJO0LOR7cX/RtuFbO6NqeKCw=="},"keyPub":{"type":"ECP256K1","rawData":"Am4regJZ+i3Z7QVhreQ05qVKxis00ib2l98I/T+dx/ZE"}}]},{"id":{"type":"SHA256","data":"VhstOOA/cjoZnayXN8n16W3A2Itm59H+o+aVzZuaH3s="},"inputs":[{"id":{"type":"SHA256","data":"77FJSI0TN2PKG1sSMzdxXulTgYksFnmpAhcaeQv2gT0="},"outputId":1,"signatureId":0}],"outputs":[{"address":{"data":"N7BiWVGenpHu7w7rd8vF1pmxhtnMEYE45H"},"value":{"value":"2"}},{"address":{"data":"NH5Sr4NDcLDdEd5Gc6byrA2Rxh6ndR5E2N"},"value":{"value":"2"}}],"signatures":[{"signature":{"type":"SHA256","data":"6t43t6it1LQYpGL/zpMbyvyNwHOYVe/ZEpwHJbRyMugSMpqAOBflmsRWb1dZMOfzsZ119MYaWdAIbPtxsa54Eg=="},"keyPub":{"type":"ECP256K1","rawData":"Am4regJZ+i3Z7QVhreQ05qVKxis00ib2l98I/T+dx/ZE"}}]},{"id":{"type":"SHA256","data":"h4PKFa0Iu9+mCPnJyOO2uYQiHJoGUk2CKSUB/mqe/DU="},"inputs":[{"id":{"type":"SHA256","data":"hXBet5nSZi/9ySWdZQNQcfWPF5O0aDQS5cdVYSE7cs8="},"outputId":1,"signatureId":0}],"outputs":[{"address":{"data":"N5U2TQWMWfXF5z5bBVpnT4KzxpkYuFGnho"},"value":{"value":"1"}},{"address":{"data":"NH5Sr4NDcLDdEd5Gc6byrA2Rxh6ndR5E2N"},"value":{"value":"1"}}],"signatures":[{"signature":{"type":"SHA256","data":"JUrNTRKQ1i0pTb0AGBh0HkOj5aKiz9BeC8N4/XbCql7YMAFZCPA+gm9xJuwoNKdm/Ttf9jtde4ugDwzmdYDCAA=="},"keyPub":{"type":"ECP256K1","rawData":"Am4regJZ+i3Z7QVhreQ05qVKxis00ib2l98I/T+dx/ZE"}}]},{"id":{"type":"SHA256","data":"lOGg7isE2tJGMVHjXMz2bwlNhrewHr4Wk/jo278jNt4="},"inputs":[{"id":{"type":"SHA256","data":"MEidA+ycYRTqx2NeYFEaGBNohYQLoffbrud4ZBJG3xY="},"outputId":1,"signatureId":0}],"outputs":[{"address":{"data":"NEcd7W2csRbQJz7NeTNGHGoNoGvrw7EkvL"},"value":{"value":"6"}},{"address":{"data":"NH5Sr4NDcLDdEd5Gc6byrA2Rxh6ndR5E2N"},"value":{"value":"7"}}],"signatures":[{"signature":{"type":"SHA256","data":"xIz2H6rx0tD+yoVuBc1U7R77siY2Q3tUwDWaay7KnicWhg4koMmovKuiP6SK523RfPzOjIkjlZpVa7zJi0IhKQ=="},"keyPub":{"type":"ECP256K1","rawData":"Am4regJZ+i3Z7QVhreQ05qVKxis00ib2l98I/T+dx/ZE"}}]},{"id":{"type":"SHA256","data":"osN9+bFKntO2KAVi6XjhLnJwnyUzOJCdDkp4xPMqezY="},"inputs":[{"id":{"type":"SHA256","data":"GTgLuFXzp8IxjChAdaIPdTUZrznPwDvQT6uwM7qmi8Y="},"outputId":1,"signatureId":0}],"outputs":[{"address":{"data":"N5U2TQWMWfXF5z5bBVpnT4KzxpkYuFGnho"},"value":{"value":"25"}},{"address":{"data":"NH5Sr4NDcLDdEd5Gc6byrA2Rxh6ndR5E2N"},"value":{"value":"26"}}],"signatures":[{"signature":{"type":"SHA256","data":"rqSUgPLWbsf8vIllAi3D2izYJbxcCs/08+Iim26flV5pSqSrcfv8WuHm1NbvAODOsVtwTTwDNnw42MlEgL9QDw=="},"keyPub":{"type":"ECP256K1","rawData":"Am4regJZ+i3Z7QVhreQ05qVKxis00ib2l98I/T+dx/ZE"}}]}],"coinbase":{"id":{"type":"SHA256","data":"wSdT7oEUxvkjqrc6vfCbij6AfirHvGebfVd4LmN8O6Y="},"outputs":[{"address":{"data":"NH5Sr4NDcLDdEd5Gc6byrA2Rxh6ndR5E2N"},"value":{"value":"100"},"data":""}],"coinbaseHeight":152390}}]}
```

### Get total amount of block
**URL**: `/total_nb_blocks`

**method**: `GET`

**exemple**:
```
$ curl http://52.47.129.155:8080/total_nb_blocks
24414
```

## Transaction
### Get Transaction by id
**URL**: `/transaction/`

**method**: `POST`

**data**:
```
{
    type: SHA256,
    data: <transaction_id>
}
```

**example**:
```
$ curl localhost:8080/transaction/ -d '{"type":"SHA256", "data":"h75iSkWlN53Z78ejSHEy4wx5qV4glWnNwqK8q+ICBFQ="}'
{"id":{"type":"SHA256","data":"h75iSkWlN53Z78ejSHEy4wx5qV4glWnNwqK8q+ICBFQ="},"inputs":[{"id":{"type":"SHA256","data":"iN0DNP+1hOlA3CB2c15BUHfC8OVnFd4MDvfPPhZg78g="},"outputId":0,"signatureId":0},{"id":{"type":"SHA256","data":"tDfNuNuzlkuy3FYcVoCvDLMmifTeXEPbgah1xHHjHU0="},"outputId":1,"signatureId":0}],"outputs":[{"address":{"data":"NH5Sr4NDcLDdEd5Gc6byrA2Rxh6ndR5E2N"},"value":{"value":"1"}},{"address":{"data":"N5U2TQWMWfXF5z5bBVpnT4KzxpkYuFGnho"},"value":{"value":"2"}}],"signatures":[{"signature":{"type":"SHA256","data":"hOGo2sNhLVYgUmVLXGzUbaCvaP5xrqX9OmNpKmTsUC9T51MpA+mbf6XvkxfKD9euCMj7gvkYqFUH/k4+vs8sAw=="},"keyPub":{"type":"ECP256K1","rawData":"AgNEbgj8H4laaNQlIwH0rXiG+KQWEjDV5cBHCMkQ5X+i"}},{"signature":{"type":"SHA256","data":"NWGRS4rLxCf2sXCKFzMS7jt7nzH1DWy7PwVr+Jcn3dBFZCdRiE+9shSj5lXwgmzjnTqrhllGBjR0KAtsWrDJ+w=="},"keyPub":{"type":"ECP256K1","rawData":"AgNEbgj8H4laaNQlIwH0rXiG+KQWEjDV5cBHCMkQ5X+i"}}]}
```

### Get a list of transaction
**URL**: `/list_transactions/:address`

**URL Parameter**: `address` address of wallet to list transaction

**method**: `GET`

get unspent transaction (search by output)

**exemple**:
```

```

### Get total number of transaction
**URL**: `/total_nb_transactions`

**method**: `GET`

**exemple**:
```
$ curl http://52.47.129.155:8080/total_nb_transactions
129885
```
### Create transaction
**URL**: `/create_transaction/:fee`

**method**: `POST`

**data**:
```
{
    key_pub: <public key>,
    outputs: [
        {
            address: {data: <address>},
            value: {value: <value>},
            data: <data>
        },
        ...
    ],
    fee: <optional amount for fee>
}
```

**exemple**:
```
```

### publish a transaction
**URL**: `/publish`

**method**: `POST`

## Misc.
### Get the balance of a wallet
**URL**: `/balance

**URL Parameter**: `address` address of a wallet

**method**: `POST`

**exemple**:
```
$ curl http://52.47.129.155:8080/balance/NH5Sr4NDcLDdEd5Gc6byrA2Rxh6ndR5E2N
{"value":"5289"}
```

### Check if a bot is alive
**URL**: `/ready`

**method**: `GET`

**exemple**:
```
$ curl http://52.47.129.155:8080/ready
{ok: 1}
```

### Get peers list from a bot
**URL**: `/peers`

**method**: `GET`

get the status of peer which are connected to the bot

**exemple**:
```
$ curl http://52.47.129.155:8080/peers
{"peers":[{"endpoint":"41.92.67.146","keyPub":{"type":"ECP256K1","rawData":"A07LyMvHZ48vtxuTPOE2uxBHJKAG0cLgCr3yzXYVN6Lz"},"port":1337,"status":"DISCONNECTED","nextUpdate":{"data":1562758223}},{"endpoint":"196.118.236.185","keyPub":{"type":"ECP256K1","rawData":"A7v3gWeIDJqJ3iM0bPNBRVi4JcY0I5dVRN8afG0d+Y/6"},"port":1337,"status":"DISCONNECTED","nextUpdate":{"data":1562756598}},{"endpoint":"35.181.53.31","keyPub":{"type":"ECP256K1","rawData":"Axp4mijnbySixTwaBvToR62oW+CeZZnNTOgjsoE8wB8o"},"port":1337,"status":"CONNECTED","nextUpdate":{"data":1562709102}},{"endpoint":"35.180.92.249","keyPub":{"type":"ECP256K1","rawData":"A+zB96s6y26rZ0yzOKxftNZdsByILW8RfT4f9gjzrzKE"},"port":1337,"status":"CONNECTED","nextUpdate":{"data":1562751678}},{"endpoint":"52.47.181.178","keyPub":{"type":"ECP256K1","rawData":"A5mc7ff4DMn9DyLq2qcbAUMyHfmzlvKKbLWuXHlQmuCD"},"port":1337,"status":"DISCONNECTED","nextUpdate":{"data":1562758223}},{"endpoint":"52.47.188.185","keyPub":{"type":"ECP256K1","rawData":"A5roo8If5GXf7mFY6tTcD2K/8oDi7BL2nhXvyC4fKlAC"},"port":1337,"status":"CONNECTED","nextUpdate":{"data":1562754365}},{"endpoint":"35.180.230.24","keyPub":{"type":"ECP256K1","rawData":"A8RPy0+a+/sO4/mKRA42Q2iBMArV6QAPddbajx2ZeHkg"},"port":1337,"status":"DISCONNECTED","nextUpdate":{"data":1562732201}},{"endpoint":"35.180.243.45","keyPub":{"type":"ECP256K1","rawData":"Anjy3xeuFab7G1v7A8HBPPaTO/oYqVGdb0nOJevecP/h"},"port":1337,"status":"CONNECTED","nextUpdate":{"data":1562756626}},{"endpoint":"35.180.97.219","keyPub":{"type":"ECP256K1","rawData":"AutRIkzh0XWicHr/eVNN9Y4vVWdEj0r14qJfuBdIZRh6"},"port":1337,"status":"DISCONNECTED","nextUpdate":{"data":1562706691}},{"endpoint":"35.180.58.83","keyPub":{"type":"ECP256K1","rawData":"Ah9nY/qXNsYnBayPVqzmEoAKCI7H9eslkuZDHg/J0cjE"},"port":1337,"status":"DISCONNECTED","nextUpdate":{"data":1562754438}},{"endpoint":"35.180.254.136","keyPub":{"type":"ECP256K1","rawData":"Am4regJZ+i3Z7QVhreQ05qVKxis00ib2l98I/T+dx/ZE"},"port":1337,"status":"CONNECTED","nextUpdate":{"data":1562750776}},{"endpoint":"35.181.59.241","keyPub":{"type":"ECP256K1","rawData":"AjEwX5KpArTEEpDSeeXAhKRQ1IlpV1IVTPzvxOoPio4u"},"port":1337,"status":"DISCONNECTED","nextUpdate":{"data":1562754438}},{"endpoint":"52.47.208.228","keyPub":{"type":"ECP256K1","rawData":"At2zvnKjc8FhInceEx4LWJ3Yy6DyD83MR3jlCneoZZpP"},"port":1337,"status":"CONNECTED","nextUpdate":{"data":1562754865}},{"endpoint":"15.188.8.94","keyPub":{"type":"ECP256K1","rawData":"AgNEbgj8H4laaNQlIwH0rXiG+KQWEjDV5cBHCMkQ5X+i"},"port":1337,"status":"DISCONNECTED","nextUpdate":{"data":1562754438}},{"endpoint":"35.180.198.126","keyPub":{"type":"ECP256K1","rawData":"AjASMFMjkbRWV4DBtU064FFSH0bgHFW/xhrg9pUNbc9D"},"port":1337,"status":"DISCONNECTED","nextUpdate":{"data":1562706681}}]}
```

### Get info from a bot
**URL**: `/status`

**method**: `GET`

* uptime: time the bot is alive (sec)
* utime: cpu time used by the bot (sec)
* stime: cpu time used by the system (sec)
* cpuUsage: percentage of cpu used (%)
* memory: max resident memory used (bytes)
* netIn: messages received
* netOut: messages sent
* lastBlockTs: time since last block (sec)
* currentHeight: current height in blockchain
* totalSpace: remaining space on disk (bytes)
* usedSpace: space used on disk (bytes)
* totalInode: remaining inode on disk
* usedInode: inode used on disk
* peer: count of peer indexed by status

**exemple**:
```
$ curl localhost:8080/status
{"bot":{"uptime":8,"utime":0,"stime":0,"cpuUsage":0,"memory":94456,"netIn":0,"netOut":0},"blockchain":{"lastBlockTs":1562759946,"currentHeight":161620},"fs":{"totalSpace":1473970176,"usedSpace":1033154560,"totalInode":15073280,"usedInode":1246545},"peer":{"connected":0,"connecting":0,"disconnected":0,"unreachable":1}}
```
