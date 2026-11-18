#!/usr/bin/env python3
"""No-network tests for paid-call bounds and controller response validation."""
import importlib.util
import io
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).parents[1]/'tools'))
spec=importlib.util.spec_from_file_location('jev_play',Path(__file__).parents[1]/'tools/jev_play.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
OBS={'sequence':1,'completed':1,'game':{'mode':4,'scene':1,'fighters':[{'slot':0,'x':0,'y':0,'vy':0,'airborne':0,'jumpsUsed':0}]}}

class HarnessTests(unittest.TestCase):
    def invoke(self,root,cost):
        with patch.object(sys,'argv',['jev_play',str(root),'--device','test','--max-cost',str(cost),'--max-calls','1']),patch.dict(os.environ,{'OPEN_ROUTER_API_KEY':'test-placeholder'}),patch.object(module,'snapshot'),patch.object(module,'state',return_value=OBS):
            module.main()

    def test_previous_charges_prevent_new_request(self):
        with tempfile.TemporaryDirectory() as path:
            root=Path(path);(root/'decisions-prior.jsonl').write_text(json.dumps({'charge':.99})+'\n')
            with patch.object(module.urllib.request,'urlopen') as request:
                self.invoke(root,.995)
                request.assert_not_called()

    def test_transient_retry_cannot_exceed_remaining_budget(self):
        with tempfile.TemporaryDirectory() as path:
            root=Path(path);(root/'decisions-prior.jsonl').write_text(json.dumps({'charge':.99})+'\n')
            error=module.urllib.error.HTTPError('https://example.invalid',429,'Rate limited',None,None)
            with patch.object(module.urllib.request,'urlopen',side_effect=error) as request:
                with self.assertRaisesRegex(RuntimeError,'status 429'):self.invoke(root,1)
                self.assertEqual(request.call_count,1)

    def test_invalid_action_never_reaches_controller(self):
        with tempfile.TemporaryDirectory() as path:
            root=Path(path)
            response={'usage':{'cost':.0001},'answers':{'action':{'type':'choice','choice':'execute_shell'}}}
            with patch.object(module.urllib.request,'urlopen',return_value=io.BytesIO(json.dumps(response).encode())),patch.object(module,'act') as act:
                with self.assertRaisesRegex(RuntimeError,'Decision request failed'):self.invoke(root,1)
                act.assert_not_called()
            rows=[json.loads(line) for p in root.glob('decisions-*.jsonl') for line in p.read_text().splitlines()]
            self.assertEqual(rows[0]['reservedCost'],.01)
            self.assertNotIn('test-placeholder',str(rows))

    def test_paid_request_recorded_if_controller_fails(self):
        with tempfile.TemporaryDirectory() as path:
            root=Path(path)
            response={'usage':{'cost':.0001},'answers':{'action':{'type':'choice','choice':'right'}}}
            with patch.object(module.urllib.request,'urlopen',return_value=io.BytesIO(json.dumps(response).encode())),patch.object(module,'act',side_effect=TimeoutError('controller')):
                with self.assertRaises(TimeoutError):self.invoke(root,1)
            rows=[json.loads(line) for p in root.glob('decisions-*.jsonl') for line in p.read_text().splitlines()]
            self.assertEqual(rows[0]['charge'],.0001)
            self.assertEqual(rows[0]['error'],'TimeoutError')

if __name__=='__main__':unittest.main()
