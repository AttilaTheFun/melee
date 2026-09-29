"""Loopback-only UDP loss injector for real TURN transport regressions."""
import random
import selectors
import socket
import threading


class LossyTurnProxy:
    def __init__(self, target_port, loss_percent):
        self.target = ('127.0.0.1', target_port)
        self.loss = loss_percent / 100
        self.random = random.Random(20260929)
        self.stop = threading.Event()
        self.error = None
        self.seen = self.dropped = 0
        self.selector = selectors.DefaultSelector()
        self.front = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.front.bind(('127.0.0.1', 0))
        self.port = self.front.getsockname()[1]
        self.selector.register(self.front, selectors.EVENT_READ, None)
        self.peers = {}
        self.thread = threading.Thread(target=self.run, name='melee-turn-loss')
        self.thread.start()

    def run(self):
        try:
            while not self.stop.is_set():
                for key, _ in self.selector.select(0.1):
                    packet, sender = key.fileobj.recvfrom(65535)
                    self.seen += 1
                    if self.random.random() < self.loss:
                        self.dropped += 1
                        continue
                    if key.data is None:
                        upstream = self.peers.get(sender)
                        if upstream is None:
                            if len(self.peers) >= 16:
                                raise RuntimeError('Too many test TURN clients')
                            upstream = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                            upstream.connect(self.target)
                            self.peers[sender] = upstream
                            self.selector.register(upstream, selectors.EVENT_READ, sender)
                        upstream.send(packet)
                    else:
                        self.front.sendto(packet, key.data)
        except BaseException as error:
            self.error = error

    def close(self):
        self.stop.set()
        self.thread.join(timeout=2)
        if self.thread.is_alive():
            raise RuntimeError('TURN loss proxy did not stop')
        for key in list(self.selector.get_map().values()):
            key.fileobj.close()
        self.selector.close()
        print(f'TURN UDP impairment: {self.dropped}/{self.seen} datagrams dropped; sockets closed.')
        if self.error:
            raise self.error
