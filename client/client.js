const net = require('net')
const mqtt = require('mqtt-packet');

const MQTT_TYPES = {
    NONE: 0, // DEFAULT NONE 
    CONNECT: 1, // 连接请求
    CONNACK: 2, // 连接确认
    PUBLISH: 3, // 发布消息
    PUBACK: 4, // 发布确认
    PUBREC: 5, // 发布收到 (QoS2)
    PUBREL: 6, // 发布释放 (QoS2)
    PUBCOMP: 7, // 发布完成 (QoS2)
    SUBSCRIBE: 8, // 客户端向代理发起订阅请求
    SUBACK: 9, // 订阅确认
    UNSUBSCRIBE: 10, // 取消订阅
    UNSUBACK: 11, // 取消订阅确认
    PINGREQ: 12, // PING 请求
    PINGRESP: 13, // PING 响应
    DISCONNECT: 14, // 断开连接
    RETAIN: 15 // 保留
};

const testSteps = [{
    char: 'A',
    packages: [{
        type: MQTT_TYPES.CONNECT,
        length: 0,
        sleep: 0
    }, {
        type: MQTT_TYPES.CONNECT,
        length: 1,
        sleep: 0
    }, {
        type: MQTT_TYPES.CONNECT,
        length: 120,
        sleep: 0
    }, {
        type: MQTT_TYPES.CONNECT,
        length: 127,
        sleep: 0
    }]
}, {
    char: 'A',
    packages: [{
        type: MQTT_TYPES.CONNECT,
        length: 128,
        sleep: 0
    }, {
        type: MQTT_TYPES.CONNECT,
        length: 1024,
        sleep: 0
    }, {
        type: MQTT_TYPES.CONNECT,
        length: 16383,
        sleep: 0
    }]
}, {
    char: 'A',
    packages: [{
        type: MQTT_TYPES.CONNECT,
        length: 16384,
        sleep: 0
    }, {
        type: MQTT_TYPES.CONNECT,
        length: 163840,
        sleep: 0
    }, {
        type: MQTT_TYPES.CONNECT,
        length: 2097151,
        sleep: 0
    }]
}, {
    char: 'A',
    packages: [{
        type: MQTT_TYPES.CONNECT,
        length: 2097152,
        sleep: 0
    }, {
        type: MQTT_TYPES.CONNECT,
        length: 20971530,
        sleep: 0
    }, {
        type: MQTT_TYPES.CONNECT,
        length: 268435455,
        sleep: 0
    }, {
        type: MQTT_TYPES.CONNECT,
        length: 268435456,
        sleep: 0
    }]
}]
const complexSteps = [{
    char: 'A',
    packages: [{
        type: MQTT_TYPES.PUBLISH,
        length: 0,
        sleep: 0
    }, {
        type: MQTT_TYPES.CONNACK,
        length: 120,
        sleep: 0
    }, {
        type: MQTT_TYPES.DISCONNECT,
        length: 1024,
        sleep: 0
    }]
}, {
    char: 'B',
    packages: [{
        type: MQTT_TYPES.PINGREQ,
        length: 120,
        sleep: 0
    }, {
        type: MQTT_TYPES.PUBCOMP,
        length: 3970,
        sleep: 0
    }, {
        type: MQTT_TYPES.RETAIN,
        length: 1210,
        sleep: 0
    }]
}, {
    char: 'C',
    packages: [{
        type: MQTT_TYPES.PINGREQ,
        length: 120,
        sleep: 0
    }, {
        type: MQTT_TYPES.PUBCOMP,
        length: 3968,
        sleep: 0
    }, {
        type: MQTT_TYPES.RETAIN,
        length: 1210,
        sleep: 0
    }]
}, {
    char: 'D',
    packages: [{
        type: MQTT_TYPES.PINGREQ,
        length: 120,
        sleep: 0
    }, {
        type: MQTT_TYPES.PUBCOMP,
        length: 3969,
        sleep: 0
    }, {
        type: MQTT_TYPES.RETAIN,
        length: 163840,
        sleep: 0
    }]
}, {
    char: 'E',
    packages: [{
        type: MQTT_TYPES.PUBLISH,
        length: 0,
        sleep: 0
    }, {
        type: MQTT_TYPES.CONNACK,
        length: 120,
        sleep: 10000
    }, {
        type: MQTT_TYPES.DISCONNECT,
        length: 1024,
        sleep: 0
    }]
}, {
    char: 'F',
    packages: [{
        type: MQTT_TYPES.PINGREQ,
        length: 120,
        sleep: 0
    }, {
        type: MQTT_TYPES.PUBCOMP,
        length: 3970,
        sleep: 10000
    }, {
        type: MQTT_TYPES.RETAIN,
        length: 1210,
        sleep: 0
    }]
}, {
    char: 'G',
    packages: [{
        type: MQTT_TYPES.PINGREQ,
        length: 120,
        sleep: 0
    }, {
        type: MQTT_TYPES.PUBCOMP,
        length: 3968,
        sleep: 10000
    }, {
        type: MQTT_TYPES.RETAIN,
        length: 1210,
        sleep: 0
    }]
}, {
    char: 'H',
    packages: [{
        type: MQTT_TYPES.PINGREQ,
        length: 120,
        sleep: 10000,
    }, {
        type: MQTT_TYPES.PUBCOMP,
        length: 3969,
        sleep: 10000
    }, {
        type: MQTT_TYPES.RETAIN,
        length: 163848,
        sleep: 0
    }]
}]
function sleepXMicroSeconds(x) {
    return new Promise(resolve => {
        setTimeout(() => {
            resolve()
        }, x)
    })
}

class ParseTest {
    constructor() {
    }
    async Startup(port = 7010, host = '127.0.0.1') {
        console.log(host);
        return new Promise(resolve => {
            this.client = net.createConnection(port, host, () => {
                resolve(this)
            });
            this.client.on('data', this.onMesage.bind(this))
            this.client.on('error', this.onError.bind(this))
            this.client.on('close', this.onClose.bind(this))
        })
    }
    onMesage(buf) {
        console.log('接收到数据: ' + buf)
    }
    onError(err) {
        console.log('连接出错', err)
    }
    onClose() {
        console.log('连接断开')
        this.isConnect = false;
    }
    write(buf) {
        return new Promise(resolve => {
            this.client.write(buf, (err) => {
                resolve(err);
            });
        });
    }
    close() {
        this.client.end();
    }
    simplePack(char, {type, length}) {
        // console.log(char, type, length)
        let size = length;
        let i = 1;
        let tmp = [type << 4]
        do {
            let byte = size % 128;
            size = parseInt(size / 128);
            if (size > 0) {
                byte = byte | 128;
                // console.log(byte);
            } else {
                // console.log(byte);
            }
            tmp.push(byte)
            ++i;
        } while (size > 0)
        // console.log(i - 1);
        // 分配内存
        let buf = Buffer.alloc(i + length);
        for (let n = 0; n < tmp.length; ++n) {
            buf.writeUInt8(tmp[n], n);
        }
        buf.fill(char, i, length);
        // console.log(buf[0], buf.length);
        return buf;
    }
    simpleUnPack(buf) {
        let type = (buf[0] >> 4) & 0xF;
        let multiplier = 1;
        let value = 0;
        let i = 1;
        let byte = 0;
        do {
            byte = buf[i];
            value += (byte & 127) * multiplier;
            multiplier *= 128;
            if (multiplier > 128 * 128 * 128) { // 下次尺寸会超出
                break;
            }
            i++;
        } while ((byte & 128) != 0)
        return {type, value};
    }
    sendConnect() {
        let buf = mqtt.generate({
            cmd: 'connect',
            retain: false,
            clientId: 'BottleClient',
            protocolVersion: 4,
            username: 'bottle',
            password: '12312312',
            will: {
                topic: 'event/huabao/io',
                payload: Buffer.from('hello world'),
                retain: true,
                qos: 1
            }
        })
        console.log(buf);
        this.write(buf)
    }
}

async function simpleTest(client) {
    for (let i in testSteps) {
        // console.log(testSteps[i]);
        let char = testSteps[i].char;
        let packages = testSteps[i].packages;
        for (let j in packages) {
            let type = packages[j].type;
            let length = packages[j].length;
            let buf = client.simplePack(char, {type, length});
            // if (length != 268435455) continue;
            let r = client.simpleUnPack(buf);
            console.log(r);
            let result = await client.write(buf);
            console.log(result);
            await sleepXMicroSeconds(5000);
        }
        // let arr = c.simplePack('A', {type: 2, length: 268435455})
        // let r = c.simpleUnPack(arr)
    }
}

async function complexTest(client) {
    for (let i in complexSteps) {
        // if (i != 3) continue;
        // console.log(testSteps[i]);
        let char = complexSteps[i].char;
        let packages = complexSteps[i].packages;
        console.log(`开始发包: 当前为: ${i}: ${char}`);
        let arr = [];
        for (let j in packages) {
            let type = packages[j].type;
            let length = packages[j].length;
            let sleep = packages[j].sleep;
            let buf = client.simplePack(char, {type, length});
            arr.push(buf);
            if (sleep > 0) {
                let sendBuffer = Buffer.concat(arr);
                console.log('Send Length', sendBuffer.length)
                await client.write(sendBuffer);
                console.log('send complate');
                arr = [];
                await sleepXMicroSeconds(sleep);
            }
        }
        if (arr.length > 0) {
            console.log(arr);
            let sendBuffer = Buffer.concat(arr);
            console.log('Over Send Length', sendBuffer.length)
            await client.write(sendBuffer);
            console.log(i + ' Over send complate');
            arr = [];
        }
        await sleepXMicroSeconds(5000);
        // let arr = c.simplePack('A', {type: 2, length: 268435455})
        // let r = c.simpleUnPack(arr)
    }
}

async function main() {
    // let c = await new ParseTest().Startup();
    let c = await new ParseTest().Startup(7010, '172.128.128.77');
    await simpleTest(c);
    // await complexTest(c);
    // c.sendConnect();
    // c.close();
}

main();