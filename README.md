# cache-simulator
homework#1 of Advanced Computer Architecture
Cache算法设计、实现与比较
假定已知：
	Cache size: 128 KB.
	Block size: 8 Bytes 
	For a 64-bit memory address.
	Cache replacement algorithm: LRU
请用C语言编写模拟程序，当给定一个memory访问记录的时候，运行该模拟程序并统计给出：
（1）	如果使用直接映射DM方法组织Cache，Cache命中率为多少？
（2）	如果使用组关联SA方法组织Cache，当路数为2-way、4-way、8-way、16-way时，Cache命中率分别为多少？
（3）	如果使用2-way、4-way、8-way、16-way组关联SA方法组织Cache，当使用MRU路预测方法时，Cache的一次命中率（first hit）、非一次命中率（non-first hit）、总的命中率分别为多少？
（4）	如果使用2-way、4-way、8-way、16-way组关联SA方法组织Cache，当使用Multi-column预测方法时，Cache的一次命中率（first hit）、非一次命中率（non-first hit）、总的命中率分别为多少？
（5）	比较说明MRU路预测方法、Multi-column预测方法的优缺点，并给出背后的原因。
说明：
（1）	memory访问记录将以每行一个地址数字的文本文件形式给出；
（2）	完成后请提交：（i）设计思路文档；（ii）C语言源代码；（iii）记录缺失的log文件。
