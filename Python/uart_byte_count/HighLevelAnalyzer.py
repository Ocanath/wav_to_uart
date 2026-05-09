from saleae.analyzers import HighLevelAnalyzer, AnalyzerFrame


class UartByteCount(HighLevelAnalyzer):

    result_types = {
        'count': {
            'format': '{{data.n}}'
        }
    }

    def __init__(self):
        self._count = 0

    def decode(self, frame: AnalyzerFrame):
        if frame.type != 'data':
            return None
        self._count += 1
        return AnalyzerFrame('count', frame.start_time, frame.end_time, {
            'n': self._count
        })
