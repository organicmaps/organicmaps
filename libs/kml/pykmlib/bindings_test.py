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

        def make_compilation():
            c = pykmlib.CategoryData()
            c.type = pykmlib.CompilationType.Category
            c.name['default'] = 'Test category'
            c.name['ru'] = 'Тестовая категория'
            c.description['default'] = 'Test description'
            c.description['ru'] = 'Тестовое описание'
            c.annotation['default'] = 'Test annotation'
            c.annotation['en'] = 'Test annotation'
            c.image_url = 'https://localhost/123.png'
            c.visible = True
            c.author_name = 'Organic Maps'
            c.author_id = '12345'
            c.rating = 8.9
            c.reviews_number = 567
            c.last_modified = int(datetime.datetime.now().timestamp())
            c.access_rules = pykmlib.AccessRules.PUBLIC
            c.tags.set_list(['mountains', 'ski', 'snowboard'])
            c.toponyms.set_list(['12345', '54321'])
            c.languages.set_list(['en', 'ru', 'de'])
            c.properties.set_dict({'property1':'value1', 'property2':'value2'})
            return c

        category = make_compilation()

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
        bookmark.visible = True
        bookmark.nearest_toponym = '12345'
        bookmark.properties.set_dict({'bm_property1':'value1', 'bm_property2':'value2'})
        bookmark.bound_tracks.set_list([0])
        bookmark.compilations.set_list([1, 2, 3])

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
        pt1 = pykmlib.PointWithAltitude()
        pt1.set_point(pykmlib.LatLon(45.9242, 56.8679))
        pt1.set_altitude(100)
        pt2 = pykmlib.PointWithAltitude()
        pt2.set_point(pykmlib.LatLon(45.2244, 56.2786))
        pt2.set_altitude(110)
        pt3 = pykmlib.PointWithAltitude()
        pt3.set_point(pykmlib.LatLon(45.1964, 56.9832))
        pt3.set_altitude(pykmlib.invalid_altitude)
        track.points_with_altitudes.set_list([pt1, pt2, pt3])
        track.visible = True
        track.nearest_toponyms.set_list(['12345', '54321', '98765'])
        track.properties.set_dict({'tr_property1':'value1', 'tr_property2':'value2'})

        compilations = pykmlib.CompilationList()
        compilation = make_compilation()
        compilation.compilation_id = 1
        compilations.append(compilation)
        collection = make_compilation()
        collection.compilation_id = 2
        collection.type = pykmlib.CompilationType.Collection
        compilations.append(collection)
        day = make_compilation()
        day.compilation_id = 3
        day.type = pykmlib.CompilationType.Day
        compilations.append(day)

        file_data = pykmlib.FileData()
        file_data.server_id = 'AAAA-BBBB-CCCC-DDDD'
        file_data.category = category
        file_data.bookmarks.append(bookmark)
        file_data.tracks.append(track)
        file_data.compilations = compilations

        pykmlib.set_bookmarks_min_zoom(file_data, 1.0, 19)

        s = pykmlib.export_kml(file_data)
        imported_file_data = pykmlib.import_kml(s)
        self.assertEqual(file_data, imported_file_data)


if __name__ == "__main__":
    unittest.main()
