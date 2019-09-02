const mqtt = require('mqtt')

let client = mqtt.connect('mqtt://172.128.128.77:7010', {
    clientId: 'BottleClient',
    username: 'huabao101',
    password: 'ReadBottle12#',
    will: {
        topic: 'event/huabao/io',
        payload: 'abcdefghijklmn',
        qos: 1,
        retain: 1
    }
})
async function onconnected(){
    console.log('on connected')
}
client.on('connect', onconnected)
client.on('message', function (topic, message) {
    console.log(topic, message);
})    
client.on("error",(err)=>{
    console.log("err",err)
})