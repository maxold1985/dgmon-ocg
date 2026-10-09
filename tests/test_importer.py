import importlib.util
from pathlib import Path
script=Path(__file__).resolve().parents[1]/'tools'/'sync_wikimon.py'
spec=importlib.util.spec_from_file_location('sync_wikimon',script)
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
fixture='''<table class="wikitable"><tbody><tr><th>No.</th><th>Card Name</th></tr>
<tr><td><a href="/St-1">St-1</a></td><td>Agumon</td><td>Blue</td><td>III</td><td>Reptile</td><td>Vaccine</td><td>NSp</td><td>A</td><td>360</td><td>230</td><td>160</td></tr>
<tr><td>St-1</td><td>アグモン</td><td>ブルー</td><td>III</td><td>...</td><td>..</td><td>..</td><td>A</td><td>360</td><td>230</td><td>160</td></tr></tbody></table>
<table><tr><th>Card Number</th><th>Card Name</th><th>Japanese</th><th>Type</th></tr>
<tr><td>St-49</td><td>Offense Plug-In A</td><td>攻撃プラグインＡ</td><td>Item</td></tr></table>'''
found=m.extract_set(fixture,'Starter Ver. 1')
assert len(found)==2,found
assert found['St-1']['battle_type']=='A' and found['St-1']['attack_b']=='230',found
assert found['St-49']['kind']=='Option' and found['St-49']['name_jp']=='攻撃プラグインＡ',found
print('Importer test PASS')
fixture2='''<table><tr><td>Bo-704</td><td>Bearmon</td><td>Blue</td><td>III</td><td>Beast</td><td>Vaccine</td><td>NSp</td><td>A</td><td>370</td><td>270</td><td>150</td><td>-010</td></tr></table>'''
b=m.extract_set(fixture2,'Booster 15')
assert b['Bo-704']['attack_c']=='150',b
print('Importer card-pass test PASS')
