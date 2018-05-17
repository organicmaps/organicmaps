import unittest
import datetime
import pykmlib

class PyKmlibAdsTest(unittest.TestCase):

    def test_smoke(self):
        classificator_file_str = ''
        with open('./data/classificator.txt', 'r') as classificator_file:
            classificator_file_str = classificator_file.read()

        types_file_str = ''
        with open('./data/types.txt', 'r') as types_file:
            types_file_str = types_file.read()

        pykmlib.load_classificator_types(classificator_file_str, types_file_str)

        category = pykmlib.CategoryData()
        category.name['default'] = 'Test category'
        category.name['ru'] = 'Тестовая категория'
        category.description['default'] = 'Test description'
        category.description['ru'] = 'Тестовое описание'
        category.annotation['default'] = 'Test annotation'
        category.annotation['en'] = 'Test annotation'
        category.image_url = 'https://localhost/123.png'
        category.visible = True
        category.author_name = 'Maps.Me'
        category.author_id = '12345'
        category.rating = 8.9
        category.reviews_number = 567
        category.last_modified = int(datetime.datetime.now().timestamp())
        category.access_rules = pykmlib.AccessRules.PUBLIC
        category.tags.set_list(['mountains', 'ski', 'snowboard'])
        category.cities.set_list([pykmlib.LatLon(35.2424, 56.2164), pykmlib.LatLon(34.2443, 46.3536)])
        category.languages.set_list(['en', 'ru', 'de'])
        category.properties.set_dict({'property1':'value1', 'property2':'value2'})

        bookmark = pykmlib.BookmarkData()
        bookmark.name['default'] = 'Test bookmark'
        bookmark.name['ru'] = 'Тестовая метка'
        bookmark.description['default'] = 'Test bookmark description'
        bookmark.description['ru'] = 'Тестовое описание метки'
        bookmark.feature_types.set_list([
            pykmlib.classificator_type_to_index('historic-castle'),
            pykmlib.classificator_type_to_index('historic-memorial')])
        bookmark.custom_name['default'] = 'Мое любимое место'
        bookmark.custom_name['en'] = 'My favorite place'
        bookmark.color.predefined_color = pykmlib.PredefinedColor.BLUE
        bookmark.color.rgba = 0
        bookmark.icon = pykmlib.BookmarkIcon.HOTEL
        bookmark.viewport_scale = 15
        bookmark.timestamp = int(datetime.datetime.now().timestamp())
        bookmark.point = pykmlib.LatLon(45.9242, 56.8679) 
        bookmark.bound_tracks.set_list([0])

        layer1 = pykmlib.TrackLayer()
        layer1.line_width = 6.0
        layer1.color.rgba = 0xff0000ff
        layer2 = pykmlib.TrackLayer()
        layer2.line_width = 7.0
        layer2.color.rgba = 0x00ff00ff

        track = pykmlib.TrackData()
        track.local_id = 1
        track.name['default'] = 'Test track'
        track.name['ru'] = 'Тестовый трек'
        track.description['default'] = 'Test track description'
        track.description['ru'] = 'Тестовое описание трека'
        track.timestamp = int(datetime.datetime.now().timestamp())
        track.layers.set_list([layer1, layer2])
        track.points.set_list([
        	pykmlib.LatLon(45.9242, 56.8679),
        	pykmlib.LatLon(45.2244, 56.2786),
        	pykmlib.LatLon(45.1964, 56.9832)])

        file_data = pykmlib.FileData()
        file_data.server_id = 'AAAA-BBBB-CCCC-DDDD'
        file_data.category = category
        file_data.bookmarks.append(bookmark)
        file_data.tracks.append(track)

        s = pykmlib.export_kml(file_data)
        imported_file_data = pykmlib.import_kml(s)
        self.assertEqual(file_data, imported_file_data)


if __name__ == "__main__":
    unittest.main()
