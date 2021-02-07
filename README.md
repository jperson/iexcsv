Extract trades from IEX TOPS v1.6 files (https://iextrading.com/trading/market-data/#tops) and write to csv

Build:
  make

Run:
  ./iex2csv -i <input .pcap file> -o out.csv

CSV format:
  timestamp,symbol,price,size

The decimal point for price is an implied by position (always 4 decimal places)
and does not explicitly appear in the field.  For example, 123400 = $12.34
