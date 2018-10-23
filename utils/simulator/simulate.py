import requests
import json
import random as rd
import numpy as np
from urllib import parse
from datetime import datetime
from threading import Timer


array_keys = []
number_keys = rd.randint(10, 100)
max_sec_between_activity = 60*10
# => a bot sends a transaction every 5.47 sec (expectancy)


def get_transactions_by_address(address):
    url_list = 'http://127.0.0.1:8080/list_transactions?address='
    url_transactions = url_list + parse.quote(address)
    response_transactions = requests.post(url_transactions)
    response_transactions_data = json.loads(response_transactions.content)
    if 'unspentTransactions' in response_transactions_data:
        return response_transactions_data['unspentTransactions']
    else:
        return json.loads('{}')

def create_keys():
    url_generate_keys = 'http://127.0.0.1:8080/generate_keys'
    response_generate_keys = requests.post(url_generate_keys)
    response_generate_keys_data = json.loads(response_generate_keys.content)
    generated_address = str(response_generate_keys_data['address']['data'])
    generated_keyPriv = str(response_generate_keys_data['keyPriv']['data'])
    return (generated_address, generated_keyPriv)

def query_faucet(address):
    url_faucet = 'http://127.0.0.1:8080/faucet_send?address='
    response_faucet = requests.post(url_faucet + parse.quote(address))
    return response_faucet

def generated_keyPriv(transaction_data):
    url_generated_keyPriv = "http://127.0.0.1:8080/publish_transaction"
    response_transaction = requests.post(url_generated_keyPriv, data = transaction_data)
    return response_transaction.content


def schedule_transaction(address, keypriv):
    random_sec = rd.randint(1, max_sec_between_activity)
    t = Timer(random_sec, send_transaction, [address, keypriv])
    t.start()

def select_random_elements(list_elements, number_elements):
    list_elements_copy = list_elements.copy()
    rd.shuffle(list_elements_copy)
    return list_elements_copy[:number_elements]

def send_transaction(address, keypriv):
    response_transactions_data = get_transactions_by_address(address)
    if len(response_transactions_data) >= 1:
        # CREATE NEW TRANSACTION INPUTS
        number_inputs = rd.randint(1, len(response_transactions_data))
        inputs_array = select_random_elements(response_transactions_data, number_inputs)
        inputs_str = ""
        total_available_input = 0
        for tr in inputs_array:
            total_available_input += int(tr['value'])
            inputs_str += '"' + tr['transactionId'] + '", '
        if total_available_input > 0: 
            inputs_str = inputs_str[:-2]
            # CREATE TRANSACTION OUTPUTS
            number_ouputs = rd.randint(1, number_keys)
            outputs_array = select_random_elements(array_keys, number_ouputs)
            outputs_str = ""
            for ouput in outputs_array:
                addr = ouput[0]
                value = int(np.random.uniform() * total_available_input)
                if addr != address and value > 0:
                    outputs_str += '{"address":{"type":"SHA256","data":"' + addr + '"},"value":{"value":"' + str(value) + '"}}, '
                    total_available_input -= value
                    if total_available_input == 0:
                        break
            if total_available_input > 0:
                # CHANGE
                outputs_str += '{"address":{"type":"SHA256","data":"' + address + '"},"value":{"value":"' + str(total_available_input) + '"}}'
            else:
                # REMOVE COMMA
                outputs_str = outputs_str[:-2]
            # SEND TRANSACTION
            data = '{"transactionsIds":[' + inputs_str + '], "outputs":[' + outputs_str + '], "fees":{"value":"0"}, "keyPriv":"' + keypriv + '"}'
            response = generated_keyPriv(data)
    # SCHEDULE NEXT TRANSACTION
    schedule_transaction(address, keypriv)


if __name__ == '__main__':
    for i in range(0, number_keys):
        keys = create_keys()
        address = keys[0]
        keypriv = keys[1]
        array_keys.append(keys)
        response_faucet = query_faucet(address).content
        schedule_transaction(address, keypriv)





