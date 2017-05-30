from setuptools import setup

setup(
    name='cbfs',
    version='0.22',
    py_modules=['cbfs','config_pb2'],
    install_requires=[
        'Click==6.6',
        'requests==2.12.3',
	'protobuf==2.6.1',
    ],
    entry_points='''
        [console_scripts]
        cbfs=cbfs:cli
    ''',
)
