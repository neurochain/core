# Rest API
## Block
### Get block by id
**URL**: `/block/id/:id`

**URL Parameter**: `id` id of the block you want info on

**method**: `GET`

return the block with a the corresponding id

### Get block by height
**URL**: `/block/height/:height`

**URL Parameter**: `height` height of the block you want info on

**method**: `GET`

return the block corresponding to a certain height

### Get latest block
**URL**: `/last_blocks/:number`

**URL Parameter**: `number` quantity of block you want

**method**: `GET`

get the last N block

### Get total amount of block
**URL**: `/total_nb_blocks`

**method**: `GET`

## Transaction
### Get Transaction by id
**URL**: `/transaction:id`

**URL Parameter**: `id` id of transaction you want info on

**method**: `GET`

### Get a list of transaction
**URL**: `list_transactions/:address`

**URL Parameter**: `address` address of wallet to list transaction

**method**: `GET`

get unspent transaction (search by output)

### Get total number of transaction
**URL**: `/total_nb_transaction`

**method**: `GET`

### Create transaction
**URL**: `/create_transaction`

**method**: `POST`

### publish a transaction
**URL**: `/publish`

**method**: `POST`

## Misc.
### Get the balance of a wallet
**URL**: `/balance/:address`

**URL Parameter**: `address` address of a wallet

**method**: `GET`

### Check if a bot is alive
**URL**: `/ready`

**method**: `GET`

### Get info from a bot
**URL**: `/peers`

**method**: `GET`

get the status of peer which are connected to the bot