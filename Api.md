# Rest API
## Block
### Get block by id
**Method**: `POST`

**URL**: `/block/id`

return the block with the corresponding id

**data**:
```
{
    data: <block_id>
}
```

**example**
```
$ curl localhost:8080/block/id -d '{"data":"Fapqu5uBJp9PLip7loZWwnt6Rpuk3pnwvlQNUaF2mIg="}'
{
 "header": {
  "id": {
   "data": "Fapqu5uBJp9PLip7loZWwnt6Rpuk3pnwvlQNUaF2mIg="
  },
  "timestamp": {
   "data": 1583500807
  },
  "previousBlockHash": {
   "data": "NQOmi7r2NvX/16x3PaFO7f78HvDXfyGSene/MnNma/M="
  },
  "author": {
   "signature": {
    "data": "2P2xMQI0i22W3z8/ezroEdWekY1cHkYyQCmvg3EEUzWF9vXRo/LtuQs5EknZDwk0FcvsYwRzRDKurNJ8rOhHkg=="
   },
   "keyPub": {
    "rawData": "A35FDjWC1WDD2BIV/oXHAcXbjpcxuWqDKZmepm5GoJyl"
   }
  },
  "height": 2331081
 },
 "transactions": [
  {
   "id": {
    "data": "37JYz1pcAxigI7+xEmfOgHtRywWdORN2bt3cltlCNjM="
   },
   "inputs": [
    {
     "keyPub": {
      "rawData": "A8TtAKDkkTIQC0j9XG1j0gFseniJSOh2GA3Sfh+C8o36"
     },
     "value": {
      "value": "100"
     },
     "signature": {
      "data": "oqqAgI+YsqxEtVqsM2YZUJDUqdk8oYNAvjkYKGEyjOshCLRf9lHirtj1X4DhrJ/v5DUbsmebCTBu/NcgwhS0Xg=="
     }
    }
   ],
   "outputs": [
    {
     "keyPub": {
      "rawData": "A/B0B0Lt4PVC5oLE9DoOW7jwcStDKvBAcaaRWoUhMJfP"
     },
     "value": {
      "value": "100"
     }
    }
   ],
   "lastSeenBlockId": {
    "data": "NQOmi7r2NvX/16x3PaFO7f78HvDXfyGSene/MnNma/M="
   }
  },
  {
   "id": {
    "data": "7fhKBoFwuyRjQJYh8N4NceX24mfvYco+XWDUV/yM7Ys="
   },
   "inputs": [
    {
     "keyPub": {
      "rawData": "A2SREP/2E+FH4cax+lskviIrvBPWJPf/6WO133HEwJ/2"
     },
     "value": {
      "value": "100"
     },
     "signature": {
      "data": "HsLkkhOEwqZffLCjVNlzh7Q7K+KJWk//AblKou914JJFAe+Pm0Zpe+qBIq++P4Tnej9fYlr6geAx8BzZReNuFg=="
     }
    }
   ],
   "outputs": [
    {
     "keyPub": {
      "rawData": "AxeIKYQej4u7lxy2mk3IRymMa0bVbFr0bg83ZO4Z/JqA"
     },
     "value": {
      "value": "100"
     }
    }
   ],
   "lastSeenBlockId": {
    "data": "NQOmi7r2NvX/16x3PaFO7f78HvDXfyGSene/MnNma/M="
   }
  },
  {
   "id": {
    "data": "KO2CGHAkRz1WY4nSMq8VwUfURpd/2pgbWYpU0SiRDfk="
   },
   "inputs": [
    {
     "keyPub": {
      "rawData": "A9r7EJFJbfWcMLeiWHLLXHnRZ0KLZ0uToN6qVjjSnp+3"
     },
     "value": {
      "value": "100"
     },
     "signature": {
      "data": "UqKvEKEYdbQN7kX/WEFX8zLBSFm1J4NaxNU3ynmNILPP5Kpmk+N97MsLdwDiKyO1FR7ct+D8ZPmaC0HmrJ8NfQ=="
     }
    }
   ],
   "outputs": [
    {
     "keyPub": {
      "rawData": "AkDQ28cmXMr5xcTXUqxw+J3Ia4FAMs8B+nWyK+iekEAL"
     },
     "value": {
      "value": "100"
     }
    }
   ],
   "lastSeenBlockId": {
    "data": "NQOmi7r2NvX/16x3PaFO7f78HvDXfyGSene/MnNma/M="
   }
  },
  {
   "id": {
    "data": "WoK89r2Zf9icFhL4oCCIUr9AIpTswuJV1wab+4NXsmw="
   },
   "inputs": [
    {
     "keyPub": {
      "rawData": "AwATWyNVEmYELf1MfCMA/efzWuw2ChocfxKFGrS0Q/x+"
     },
     "value": {
      "value": "100"
     },
     "signature": {
      "data": "0dL9u6ZnnyalNFG0ikgfi4MwW4pTE6Khxhmp+3TSbmh3L+8tkYOlXdKuJu8egYYhm3youuQHCqdan+vpfzG68Q=="
     }
    }
   ],
   "outputs": [
    {
     "keyPub": {
      "rawData": "A35FDjWC1WDD2BIV/oXHAcXbjpcxuWqDKZmepm5GoJyl"
     },
     "value": {
      "value": "100"
     }
    }
   ],
   "lastSeenBlockId": {
    "data": "NQOmi7r2NvX/16x3PaFO7f78HvDXfyGSene/MnNma/M="
   }
  },
  {
   "id": {
    "data": "aojLDmlVow1hjesyJ2XBXgdTi5YZgV6f9xY3Z0Opy7c="
   },
   "inputs": [
    {
     "keyPub": {
      "rawData": "A6EnLlJ5FjMOruJF7h2WdCP+P4BMgyRMjAYj8nW2BO6S"
     },
     "value": {
      "value": "100"
     },
     "signature": {
      "data": "E3RTvRxVgsqdA7JNf/gxHDLfdqE/HM6tIx1Z5zZcBiTTqBM2asfHGwA54BVtwbyRiWSF+35N4Ym8excSDOUpkQ=="
     }
    }
   ],
   "outputs": [
    {
     "keyPub": {
      "rawData": "A/De3pk8zUIIcrMZkDHkdW7Y7n92R2aHaub2C4CYvQ8E"
     },
     "value": {
      "value": "100"
     }
    }
   ],
   "lastSeenBlockId": {
    "data": "NQOmi7r2NvX/16x3PaFO7f78HvDXfyGSene/MnNma/M="
   }
  }
 ],
 "coinbase": {
  "id": {
   "data": "2+JfOGUAyvpQm9wNMbcFaeiE+8YUVizx2c+mAPBo2IY="
  },
  "outputs": [
   {
    "keyPub": {
     "rawData": "A35FDjWC1WDD2BIV/oXHAcXbjpcxuWqDKZmepm5GoJyl"
    },
    "value": {
     "value": "100000000000"
    },
    "data": ""
   }
  ],
  "lastSeenBlockId": {
   "data": "NQOmi7r2NvX/16x3PaFO7f78HvDXfyGSene/MnNma/M="
  }
 }
}
```

### Get block by height
**method**: `GET`

**URL**: `/block/height/:height`

**URL Parameter**: `height` height of the block you want info on

return the block corresponding to a certain height

**example**:
```
$ curl localhost:8080/block/height/0
{
 "header": {
  "id": {
   "data": "WPyMrBdGUdN+jgJXhZ9tAZbWmXw8ORbB31cHpuT27Kg="
  },
  "timestamp": {
   "data": 1576507564
  },
  "previousBlockHash": {
   "data": ""
  },
  "author": {
   "signature": {
    "data": "BArxHBLhjwxnE5C2QnCxx4WUNJEkWgQXg7BquEMms7C931yYpek3CLNIMED2oi1U8FFpNLrswj8X719nKNjVTw=="
   },
   "keyPub": {
    "rawData": "A6EnLlJ5FjMOruJF7h2WdCP+P4BMgyRMjAYj8nW2BO6S"
   }
  },
  "height": 0
 },
 "coinbase": {
  "id": {
   "data": "GUJ24F0Ynrytcf6dw/BZePns7goY3qHgv5P/SyP+Djw="
  },
  "outputs": [
   {
    "keyPub": {
     "rawData": "A6EnLlJ5FjMOruJF7h2WdCP+P4BMgyRMjAYj8nW2BO6S"
    },
    "value": {
     "value": "400000000000000000"
    },
    "data": ""
   },
   {
    "keyPub": {
     "rawData": "ApYsnVU0VczRb6H/qvW3arxQaQ4N7NQ0X6ZWtA2Fa8ES"
    },
    "value": {
     "value": "400000000000000000"
    },
    "data": ""
   },
   {
    "keyPub": {
     "rawData": "Ay7bJHzqY+Wtn2RKQ0bOLe4QJ1p+4jRdI7QQsvMfxeSl"
    },
    "value": {
     "value": "400000000000000000"
    },
    "data": ""
   },
   {
    "keyPub": {
     "rawData": "AwsmLgIRhWU6/+udvSRlNbI13AkToqQwPo8wju6/b0UW"
    },
    "value": {
     "value": "400000000000000000"
    },
    "data": ""
   },
   {
    "keyPub": {
     "rawData": "AqjSGeMVxItmaBgC152V5UvzeP60oIk0QIoC0Y5HnO1h"
    },
    "value": {
     "value": "400000000000000000"
    },
    "data": ""
   },
   {
    "keyPub": {
     "rawData": "A5Rv7beIMWr9e6QY5/tIGDoeW+1UidMf4qkJW9jHCXdH"
    },
    "value": {
     "value": "400000000000000000"
    },
    "data": ""
   },
   {
    "keyPub": {
     "rawData": "AgH4AZ2IqwLnnPB/ZhvPH6XICNTUOD1ANouLYSGAtbrX"
    },
    "value": {
     "value": "400000000000000000"
    },
    "data": ""
   },
   {
    "keyPub": {
     "rawData": "A/QZkOUkyw/vcP6iZxe4Tp1CzmUiyi4rIwgwXwpy/fhq"
    },
    "value": {
     "value": "400000000000000000"
    },
    "data": ""
   },
   {
    "keyPub": {
     "rawData": "AhDbR95xqgsviFjJYbbwnclY1lt0JgkVRN6SL6sLwmLL"
    },
    "value": {
     "value": "400000000000000000"
    },
    "data": ""
   },
   {
    "keyPub": {
     "rawData": "AvJMns1Ww1u+L9cCrCbqjoxEHSuePd6LTPfeRQ4xy5ON"
    },
    "value": {
     "value": "400000000000000000"
    },
    "data": ""
   }
  ],
  "lastSeenBlockId": {
   "data": ""
  }
 }
}
```


### Get latest block
**Method**: `GET`

**URL**: `/last_blocks/:number`

**URL Parameter**: `number` quantity of block you want

get the last N block

**example**:
```
$ curl http://localhost:8080/last_blocks/2
{
 "block": [
  {
   "header": {
    "id": {
     "data": "vLe42H6rN/1qg2J39xzehsBNcTeKbSxzptBsLFTU6WE="
    },
    "timestamp": {
     "data": 1583832778
    },
    "previousBlockHash": {
     "data": "U5r1VeNKGUKjKN7s8Snqr5sEYX0cJ8ZBQY6lMEB1fcQ="
    },
    "author": {
     "signature": {
      "data": "t9s3owrIwAP7V7+KOcsJqxl+PKkFnwyNruetZh4e++KMtb2nQN554gPyWQ4O5IlWhBKSirOvKnllubGpMqW3GA=="
     },
     "keyPub": {
      "rawData": "An/C69LZ874bbX1twFk5BfatQ6ulZqElsljAatL3YuKt"
     }
    },
    "height": 2441738
   },
   "coinbase": {
    "id": {
     "data": "AOU3cGqJV4KwrLRUSt8v0SQqqLAwKBeYkp1+kk5jyZo="
    },
    "outputs": [
     {
      "keyPub": {
       "rawData": "An/C69LZ874bbX1twFk5BfatQ6ulZqElsljAatL3YuKt"
      },
      "value": {
       "value": "100000000000"
      },
      "data": ""
     }
    ],
    "lastSeenBlockId": {
     "data": "U5r1VeNKGUKjKN7s8Snqr5sEYX0cJ8ZBQY6lMEB1fcQ="
    }
   }
  },
  {
   "header": {
    "id": {
     "data": "U5r1VeNKGUKjKN7s8Snqr5sEYX0cJ8ZBQY6lMEB1fcQ="
    },
    "timestamp": {
     "data": 1583832775
    },
    "previousBlockHash": {
     "data": "r98RvYWnw06yJpCDCNfS6H1cpiqAyXAgORZM5h2FFe8="
    },
    "author": {
     "signature": {
      "data": "AFG1WJDHOicpNmIpfxn99SXc9taa6gXC+MPGardXM+YWmW+EQkLnnApe4QiZSmLjSfeDssGD1Xiwv0Ikcqi1NQ=="
     },
     "keyPub": {
      "rawData": "An/C69LZ874bbX1twFk5BfatQ6ulZqElsljAatL3YuKt"
     }
    },
    "height": 2441737
   },
   "coinbase": {
    "id": {
     "data": "Zsmu4fD2lCVI0cGmKDHd3O0tEQlJ+skXgXfKbBHE9RQ="
    },
    "outputs": [
     {
      "keyPub": {
       "rawData": "An/C69LZ874bbX1twFk5BfatQ6ulZqElsljAatL3YuKt"
      },
      "value": {
       "value": "100000000000"
      },
      "data": ""
     }
    ],
    "lastSeenBlockId": {
     "data": "r98RvYWnw06yJpCDCNfS6H1cpiqAyXAgORZM5h2FFe8="
    }
   }
  }
 ]
}
```

### Get total amount of block
**Method**: `GET`

**URL**: `/total_nb_blocks`

**exemple**:
```
$ curl http://localhost:8080/total_nb_blocks
1953888
```

## Transaction
### Get Transaction by id
**Method**: `POST`

**URL**: `/transaction/`

**data**:
```
{
    data: <transaction_id>
}
```

**example**:
```
$ curl localhost:8080/transaction/ -d '{"data":"37JYz1pcAxigI7+xEmfOgHtRywWdORN2bt3cltlCNjM="}'
{
 "id": {
  "data": "37JYz1pcAxigI7+xEmfOgHtRywWdORN2bt3cltlCNjM="
 },
 "inputs": [
  {
   "keyPub": {
    "rawData": "A8TtAKDkkTIQC0j9XG1j0gFseniJSOh2GA3Sfh+C8o36"
   },
   "value": {
    "value": "100"
   },
   "signature": {
    "data": "oqqAgI+YsqxEtVqsM2YZUJDUqdk8oYNAvjkYKGEyjOshCLRf9lHirtj1X4DhrJ/v5DUbsmebCTBu/NcgwhS0Xg=="
   }
  }
 ],
 "outputs": [
  {
   "keyPub": {
    "rawData": "A/B0B0Lt4PVC5oLE9DoOW7jwcStDKvBAcaaRWoUhMJfP"
   },
   "value": {
    "value": "100"
   }
  }
 ],
 "lastSeenBlockId": {
  "data": "NQOmi7r2NvX/16x3PaFO7f78HvDXfyGSene/MnNma/M="
 }
}
```

### Get a list of transaction
**Method**: `POST`

**URL**: `/list_transactions`

**data**:
```
{
  page: <number>;
  page_size: <number>;
  output_key_pub: {rawData: <public key>};
  input_key_pub: {rawData: <public key>};
}
```

get the list of transaction using a pagination system.

default value are used if page number / page size are omitted
(0 for page and 10 for page size)

you must specifies either an output key pub or input key pub,
the call return the transaction in which the specified key appear (act as an `or` if output and input are given)

**exemple**:
```
$ curl localhost:8080/list_transactions -d '{"page_size":2,"output_key_pub": {"rawData":"A6EnLlJ5FjMOruJF7h2WdCP+P4BMgyRMjAYj8nW2BO6S"},"input_key_pub":{"rawData":"A2SREP/2E+FH4cax+lskviIrvBPWJPf/6WO133HEwJ/2"}}'
{
 "transactions": [
  {
   "id": {
    "data": "1A7iGAWIkNd+XGhWHrJnqhbSkUtOw6RhSDkPbgM5hMU="
   },
   "inputs": [
    {
     "keyPub": {
      "rawData": "A2SREP/2E+FH4cax+lskviIrvBPWJPf/6WO133HEwJ/2"
     },
     "value": {
      "value": "100"
     },
     "signature": {
      "data": "5x7d2t+yekTveJloDfJFkvgMTo6merTllY+NkpT5YdkcVDVQjxdaKtXMyo3uIVcE3M4lMSthzE4VFCIim8CS/w=="
     }
    }
   ],
   "outputs": [
    {
     "keyPub": {
      "rawData": "A6EnLlJ5FjMOruJF7h2WdCP+P4BMgyRMjAYj8nW2BO6S"
     },
     "value": {
      "value": "100"
     }
    }
   ],
   "lastSeenBlockId": {
    "data": "xaxOtWYu87XfBF9ykt/CCI4WrizwIBKHIpjXv7j2idw="
   }
  },
  {
   "id": {
    "data": "sFVw8g/tq32xChdAOUs1BslwYssZPfa5RQk6v9vW3fs="
   },
   "inputs": [
    {
     "keyPub": {
      "rawData": "A2SREP/2E+FH4cax+lskviIrvBPWJPf/6WO133HEwJ/2"
     },
     "value": {
      "value": "100"
     },
     "signature": {
      "data": "S0/9gEOjVFgalvlcUpknODgqW+2+Ly1oR5HGVsDtaZYynGMqdqJSWI7w1iWyDLdc5RX4GTBMBViyBg2+xy4Lvw=="
     }
    }
   ],
   "outputs": [
    {
     "keyPub": {
      "rawData": "A6EnLlJ5FjMOruJF7h2WdCP+P4BMgyRMjAYj8nW2BO6S"
     },
     "value": {
      "value": "100"
     }
    }
   ],
   "lastSeenBlockId": {
    "data": "xaxOtWYu87XfBF9ykt/CCI4WrizwIBKHIpjXv7j2idw="
   }
  }
 ]
}
```

### Get total number of transaction
**Method**: `GET`

**URL**: `/total_nb_transactions`

**exemple**:
```
$ curl http://localhost:8080/total_nb_transactions
12940117
```
### Create transaction
**method**: `POST`

**URL**: `/create_transaction`

**data**:
```
{
    key_pub: {rawData: <public_key>},
    outputs: [
        {
            key_pub: {rawData: <public_key>},
            value: {value: <string>},
            data: <string>
        },
        ...
    ],
    fee: <number>
}
```

create a payload for a transaction

### publish a transaction
**Method**: `POST`

**URL**: `/publish`

**data**:
```
{
    transaction: <payload from /create_transaction>,
    signature: <signature for the payload>,
    keyPub: {rawData: <public_key>}
}
```

publish a transaction created with `/create_transaction`

the `signature` is a sha256 of the payload after being signed with a ecdsa algorithm using the secp256k1 curve
using the private key associted with the key sended in `keyPub` field

Note: you should probably use the js cli instead of trying to make it work with curl / openssl

## Misc.
### Get the balance of a wallet
**method**: `GET`

**URL**: `/balance`

**query parameter**: `pubkey`

**exemple**:
```
$ curl http://localhost:8080/balance?pubkey=A6EnLlJ5FjMOruJF7h2WdCP+P4BMgyRMjAYj8nW2BO6S
{
 "value": "413335199968488640"
}
```

### Get the balance of a wallet
**Method**: `POST`

**URL**: `/balance`

**data**:
```
{
    rawData: <public key>
}
```
**exemple**:
```
$ curl 'http://localhost:8080/balance' -d '{"rawData":"A6EnLlJ5FjMOruJF7h2WdCP+P4BMgyRMjAYj8nW2BO6S"}'
{
 "value": "413335199968488640"
}
```

### Check if a key is valid
**URL**: `/validate`

**method**: `POST`
**data**:
```
{
    rawData: <public key>
}
```

because some key pair are not as strong as other, the bot could refuse some key. This endpoint allow to test if the bot would allow a key before actually using it

it return a `406 Not acceptable` error code when key_pub is invalid

**exemple**:
```
$ curl http://localhost:8080/validate -d '{"rawData":"AlhV0BmXEIlOksGUtRrJqXy4UBfFFGue49H5TagwpSUH"}' -i
HTTP/1.1 406 Not Acceptable
Access-Control-Allow-Origin: *
Connection: Close
Content-Length: 11

invalid key
```

### Check if a bot is alive
**URL**: `/ready`

**method**: `GET`

**exemple**:
```
$ curl http://localhost:8080/ready
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

# Cookbook
## Make a transaction
To effectively emit a transaction on the network, several endpoint must be used,

- /validate to ensure your generated key is valid
- /transaction with the transaction information (outputs, ncc to send, data, fees) to create your transaction payload  
- sign your payload using secp256k1 elliptic curve
- /publish with your signature, the untouched payload and your public key

when correctly done the bot should response with the transaction id used on the network
