import sys, os

import requests
import docker
import csv
import time

PORT = 80

if len(sys.argv) < 4:
    print("Require at least 3 arguments python3 collect_test.py <testfile path> <count> <csv_output>")
    sys.exit(1)


if not os.path.exists(sys.argv[1]):
    print("File does not exist: ", sys.argv[1])
    sys.exit(1)

## Get port mapping from docker kmap container
cont = docker.from_env().containers.get('kmap')
client = docker.APIClient(base_url='unix://var/run/docker.sock')

IP_PORT = client.port(cont.id, PORT)

print("Running tests against:")
print(IP_PORT)


def runTest(file):
    start = time.time()
    resp = requests.get('http://{}:{}/{}'.format(IP_PORT[0]['HostIp'], IP_PORT[0]['HostPort'], file))
    roundtrip = time.time() - start
    if resp.status_code != 200:
        print("Resp got code: ", resp.status_code)
        print("Not recording data point")
        return False, 0
    return compareFile(file, resp.text), roundtrip

def compareFile(file, text):
    if text == open(file).read():
        return True
    else:
        return False

## RUN TESTS
output = []
for i in range(int(sys.argv[2])):
    ok, data = runTest(sys.argv[1])
    if ok:
        print("OK, time: ", data)
        output.append(data)
    else:
        print("FAILED, not recording time")

    time.sleep(0.2)

## Store data

with open(sys.argv[3], 'w', newline='') as csvfile:
    writer = csv.writer(csvfile, delimiter=' ', quotechar='|', quoting=csv.QUOTE_MINIMAL)
    writer.writerow(["Times"])
    for t in output:
        writer.writerow([str(t)])
