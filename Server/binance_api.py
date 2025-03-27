import requests
import time
import hmac
import hashlib
import urllib.parse

base_url = "http://binance.rigelow.uk"
api_key = ""
secret_key = ""
avg_price_url = "/fapi/v1/avgPrice"
newest_price_url = "/fapi/v2/ticker/price"
header = {
    "X-MBX-APIKEY": api_key,
}
def get_latest_price(cointype="BTCUSDT"):
    query_string = f"symbol={cointype}&timestamp={int(time.time()) * 1000}"
    signature = hmac.new(secret_key.encode('utf-8'), query_string.encode('utf-8'), hashlib.sha256).hexdigest()
    params = f"{query_string}&signature={signature}"
    response = requests.get(base_url + newest_price_url,headers= header, params=params)
    return response.json()
