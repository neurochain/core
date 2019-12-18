version=${1:-634566b01c75a77503cb0b624ebbc6683ab0a6f3}

docker network create neuro || echo "network already" 
if [ ! -d mongo ]; then mkdir mongo ; fi
docker pull registry.gitlab.com/neurochaintech/core/prod/release:$version
docker run -dit --name mongo --network neuro --restart always --log-opt max-size=10m --log-opt max-file=5 -v $(pwd)/mongo:/data/db  mongo:3.6.11
docker run -dit --name core  --network neuro --restart always --log-driver local --log-opt max-size=128m --log-opt max-file=4 --log-opt compress=true -p 1337:1337 registry.gitlab.com/neurochaintech/core/prod/release:$version ./main -c conf/bot.json 
