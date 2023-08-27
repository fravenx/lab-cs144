Checkpoint 5 Writeup
====================

My name: [your name here]

My SUNet ID: [your sunetid here]

I collaborated with: [list sunetids here]

I would like to thank/reward these classmates for their help: [list sunetids here]

This checkpoint took me about [n] hours to do. I [did/did not] attend the lab session.

Program Structure and Design of the Router:
[]

Implementation Challenges:
出现的bug：
1. IP数据包的ttl减1时，要重新计算校验和，否则在解析ip数据包时会出现错误
2. 在遍历router的interfaces时要使用引用的方式，否则该网卡中的ip数据包并没有被成功拿出，会出现无限循环发送ip数据包的情况

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
