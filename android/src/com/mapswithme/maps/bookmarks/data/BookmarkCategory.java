package com.mapswithme.maps.bookmarks.data;

import android.content.Context;
import android.content.res.Resources;
import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.PluralsRes;
import android.text.TextUtils;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.BookmarksPageFactory;
import com.mapswithme.util.TypeConverter;

public class BookmarkCategory implements Parcelable
{
  public static final String DESCRIPTION = "\n" +
                 "Владимир Семёнович Высоцкий\n" +
                 "Vladimir Vysotsky.jpg\n" +
                 "Владимир Высоцкий на концерте 23 апреля 1979 года. Фото Игоря Пальмина\n" +
                 "Дата рождения:\t25 января 1938\n" +
                 "Место рождения:\tМосква, СССР\n" +
                 "Дата смерти:\t25 июля 1980 (42 года)\n" +
                 "Место смерти:\tМосква, СССР\n" +
                 "Страна:\t\n" +
                 "Flag of the Soviet Union (1955–1980).svg СССР\n" +
                 "Род деятельности:\t\n" +
                 "поэт, поэт-песенник, автор-исполнитель, прозаик, актёр театра и кино, певец, " +
                 "композитор, гитарист\n" +
                 "Место работы:\t\n" +
                 "Московский драматический театр имени А. С. Пушкина\n" +
                 "Театр на Таганке\n" +
                 "Высшее образование:\t\n" +
                 "Школа-студия МХАТ\n" +
                 "Награды и премии:\tГосударственная премия СССР — 1987 Premia mvd.jpg[1][2]\n" +
                 "Подпись:\tПодпись\n" +
                 "Отец:\tСемён Владимирович Высоцкий\n" +
                 "Мать:\tНина Максимовна Высоцкая\n" +
                 "Супруг(а):\t\n" +
                 "• Изольда Жукова (Мешкова)[⇨], \n" +
                 "• Людмила Абрамова[⇨], \n" +
                 "• Марина Влади[⇨]\n" +
                 "Дети:\tАркадий Высоцкий, Никита Высоцкий[⇨]\n" +
                 "Похоронен\t\n" +
                 "Ваганьковское кладбище\n" +
                 "IMDb:\tID 0904584\n" +
                 "Логотип Викицитатника Цитаты в Викицитатнике\n" +
                 "Commons-logo.svg Владимир Семёнович Высоцкий на Викискладе\n" +
                 "Влади́мир Семёнович Высо́цкий (25 января 1938, Москва — 25 июля 1980, Москва)" +
                 " — советский поэт, актёр театра и кино, автор-исполнитель песен (бард); автор" +
                 " прозаических произведений и сценариев. Лауреат Государственной премии СССР " +
                 "(«за создание образа Жеглова в телевизионном художественном фильме „Место " +
                 "встречи изменить нельзя“ и авторское исполнение песен», 1987, посмертно).\n" +
                 "\n" +
                 "Как поэт Высоцкий реализовал себя прежде всего в жанре авторской песни. " +
                 "Первые из написанных им произведений относятся к началу 1960-х годов. " +
                 "Изначально они исполнялись в кругу друзей; позже получили широкую известность" +
                 " благодаря распространявшимся по стране магнитофонным записям. Поэзия " +
                 "Высоцкого отличалась многообразием тем (уличные, лагерные, военные, " +
                 "сатирические, бытовые, сказочные, «спортивные» песни), остротой смыслового " +
                 "подтекста и акцентированной социально-нравственной позицией автора. В его " +
                 "произведениях, рассказывающих о внутреннем выборе людей, поставленных в " +
                 "экстремальные обстоятельства, прослеживались экзистенциальные мотивы. " +
                 "Творческая эволюция Высоцкого ознаменовалась несколькими этапами. В его " +
                 "раннем творчестве преобладали уличные и дворовые песни. С середины 1960-х " +
                 "годов тематика произведений начала расширяться, а песенные циклы складываться" +
                 " в новую «энциклопедию русской жизни». В 1970-х годах значительную часть " +
                 "творчества Высоцкого составляли песни и стихотворения " +
                 "исповедально-философского характера, поэт часто обращался к вечным вопросам " +
                 "бытия.\n" +
                 "\n" +
                 "Театральная биография Высоцкого, окончившего в 1960 году Школу-студию МХАТ, " +
                 "связана главным образом с работой в Театре на Таганке. На его сцене актёр " +
                 "играл Галилея (спектакль «Жизнь Галилея», 1966), Хлопушу («Пугачёв», 1967), " +
                 "Гамлета («Гамлет», 1971), Лопахина («Вишнёвый сад», 1975), Свидригайлова " +
                 "(«Преступление и наказание», 1979). Дебют Высоцкого в кино состоялся в 1959 " +
                 "году, когда он сыграл эпизодическую роль в фильме «Сверстницы». За годы " +
                 "работы в кинематографе актёр снялся более чем в двадцати пяти фильмах. " +
                 "Кинобиография Высоцкого включает роли подпольщика Бродского («Интервенция», " +
                 "1968), зоолога фон Корена («Плохой хороший человек», 1973), капитана Жеглова " +
                 "(«Место встречи изменить нельзя», 1979), Дон Гуана («Маленькие трагедии», " +
                 "1979) и другие. Исследователи отмечали, что в сценических и экранных работах " +
                 "Высоцкого экспрессивность сочеталась с психологической достоверностью. В ряде" +
                 " спектаклей, а также в художественных фильмах, теле- и радиопостановках он " +
                 "выступал и как автор песен.\n" +
                 "\n" +
                 "При жизни Высоцкого его песни не получили в СССР официального признания. В " +
                 "1968 году в рамках газетной кампании, дискредитирующей его " +
                 "музыкально-поэтическое творчество, они были подвергнуты резкой критике. " +
                 "Вплоть до 1981 года ни одно советское издательство не выпустило книгу с его " +
                 "текстами. Цензурные ограничения частично были сняты только после смерти " +
                 "Высоцкого, когда вышел в свет сборник его поэтических произведений «Нерв» " +
                 "(составитель — Роберт Рождественский). Тем не менее цензорский контроль за " +
                 "публикациями стихов и песен Высоцкого, а также посвящённых ему " +
                 "газетно-журнальных статей продолжал действовать вплоть до перестройки. " +
                 "Легализация его творчества началась в Советском Союзе в 1986 году, когда при " +
                 "Союзе писателей СССР была создана комиссия по литературному наследию " +
                 "Высоцкого. Со второй половины 1980-х годов начался выпуск книг и собраний " +
                 "сочинений поэта, ведётся исследовательская работа, посвящённая его творчеству" +
                 ". По некоторым оценкам, Высоцкий, занимающий одно из центральных мест в " +
                 "истории русской культуры XX века, «оказал сильное влияние на формирование " +
                 "взглядов своих современников и последующих поколений».";
  private final long mId;
  @NonNull
  private final String mName;
  @Nullable
  private final Author mAuthor;
  @NonNull
  private final String mAnnotation;
  @NonNull
  private final String mDescription;
  private final int mTracksCount;
  private final int mBookmarksCount;
  private final int mTypeIndex;
  private final boolean mIsVisible;

  public BookmarkCategory(long id, @NonNull String name, @NonNull String authorId,
                          @NonNull String authorName, @NonNull String annotation,
                          @NonNull String description, int tracksCount, int bookmarksCount,
                          boolean fromCatalog, boolean isVisible)
  {
    mId = id;
    mName = name;
    mAnnotation = "asdasdasdasdasdasdasdasdasdasdasdasd";
    mDescription = DESCRIPTION;
    mTracksCount = tracksCount;
    mBookmarksCount = bookmarksCount;
    mTypeIndex = fromCatalog ? Type.CATALOG.ordinal() : Type.PRIVATE.ordinal();
    mIsVisible = isVisible;
    mAuthor = TextUtils.isEmpty(authorId) || TextUtils.isEmpty(authorName)
              ? null
              : new Author(authorId, authorName);
  }

  @Override
  public boolean equals(Object o)
  {
    if (this == o) return true;
    if (o == null || getClass() != o.getClass()) return false;
    BookmarkCategory that = (BookmarkCategory) o;
    return mId == that.mId;
  }

  @Override
  public int hashCode()
  {
    return (int)(mId ^ (mId >>> 32));
  }

  public long getId()
  {
    return mId;
  }

  @NonNull
  public String getName()
  {
    return mName;
  }

  @Nullable
  public Author getAuthor()
  {
    return mAuthor;
  }

  public int getTracksCount()
  {
    return mTracksCount;
  }

  public int getBookmarksCount()
  {
    return mBookmarksCount;
  }

  public Type getType()
  {
    return Type.values()[mTypeIndex];
  }

  public boolean isFromCatalog()
  {
    return Type.values()[mTypeIndex] == Type.CATALOG;
  }

  public boolean isVisible()
  {
    return mIsVisible;
  }

  public int size()
  {
    return getBookmarksCount() + getTracksCount();
  }

  @NonNull
  public String getAnnotation()
  {
    return mAnnotation;
  }

  @NonNull
  public String getDescription()
  {
    return mDescription;
  }

  @NonNull
  public CountAndPlurals getPluralsCountTemplate()
  {
    if (size() == 0)
      return new CountAndPlurals(0, R.plurals.objects);

    if (getBookmarksCount() == 0)
      return new CountAndPlurals(getTracksCount(), R.plurals.tracks);

    if (getTracksCount() == 0)
      return new CountAndPlurals(getBookmarksCount(), R.plurals.places);

    return new CountAndPlurals(size(), R.plurals.objects);
  }

  public static class CountAndPlurals {
    private final int mCount;
    @PluralsRes
    private final int mPlurals;

    public CountAndPlurals(int count, int plurals)
    {
      mCount = count;
      mPlurals = plurals;
    }

    public int getCount()
    {
      return mCount;
    }

    public int getPlurals()
    {
      return mPlurals;
    }
  }

  public static class Author implements Parcelable
  {
    @NonNull
    private final String mId;
    @NonNull
    private final String mName;

    public Author(@NonNull String id, @NonNull String name)
    {
      mId = id;
      mName = name;
    }

    @NonNull
    public String getId()
    {
      return mId;
    }

    @NonNull
    public String getName()
    {
      return mName;
    }

    @Override
    public String toString()
    {
      final StringBuilder sb = new StringBuilder("Author{");
      sb.append("mId='").append(mId).append('\'');
      sb.append(", mName='").append(mName).append('\'');
      sb.append('}');
      return sb.toString();
    }

    @Override
    public boolean equals(Object o)
    {
      if (this == o) return true;
      if (o == null || getClass() != o.getClass()) return false;
      Author author = (Author) o;
      return mId.equals(author.mId);
    }

    @Override
    public int hashCode()
    {
      return mId.hashCode();
    }

    @Override
    public int describeContents()
    {
      return 0;
    }

    public static String getRepresentation(@NonNull Context context, @NonNull Author author)
    {
      Resources res = context.getResources();
      return String.format(res.getString(R.string.author_name_by_prefix), author.getName());
    }

    @Override
    public void writeToParcel(Parcel dest, int flags)
    {
      dest.writeString(this.mId);
      dest.writeString(this.mName);
    }

    protected Author(Parcel in)
    {
      this.mId = in.readString();
      this.mName = in.readString();
    }

    public static final Creator<Author> CREATOR = new Creator<Author>()
    {
      @Override
      public Author createFromParcel(Parcel source)
      {
        return new Author(source);
      }

      @Override
      public Author[] newArray(int size)
      {
        return new Author[size];
      }
    };
  }

  @Override
  public String toString()
  {
    final StringBuilder sb = new StringBuilder("BookmarkCategory{");
    sb.append("mId=").append(mId);
    sb.append(", mName='").append(mName).append('\'');
    sb.append(", mAuthor=").append(mAuthor);
    sb.append(", mAnnotation='").append(mAnnotation).append('\'');
    sb.append(", mDescription='").append(mDescription).append('\'');
    sb.append(", mTracksCount=").append(mTracksCount);
    sb.append(", mBookmarksCount=").append(mBookmarksCount);
    sb.append(", mTypeIndex=").append(mTypeIndex);
    sb.append(", mIsVisible=").append(mIsVisible);
    sb.append('}');
    return sb.toString();
  }

  public static class IsFromCatalog implements TypeConverter<BookmarkCategory, Boolean>
  {
    @Override
    public Boolean convert(@NonNull BookmarkCategory data)
    {
      return data.isFromCatalog();
    }
  }

  public enum Type
  {
    PRIVATE(BookmarksPageFactory.PRIVATE, FilterStrategy.PredicativeStrategy.makePrivateInstance()),
    CATALOG(BookmarksPageFactory.CATALOG, FilterStrategy.PredicativeStrategy.makeCatalogInstance());

    private BookmarksPageFactory mFactory;
    private FilterStrategy mFilterStrategy;

    Type(@NonNull BookmarksPageFactory pageFactory, @NonNull FilterStrategy filterStrategy)
    {
      mFactory = pageFactory;
      mFilterStrategy = filterStrategy;
    }

    @NonNull
    public BookmarksPageFactory getFactory()
    {
      return mFactory;
    }

    @NonNull
    public FilterStrategy getFilterStrategy()
    {
      return mFilterStrategy;
    }
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeLong(this.mId);
    dest.writeString(this.mName);
    dest.writeParcelable(this.mAuthor, flags);
    dest.writeString(this.mAnnotation);
    dest.writeString(this.mDescription);
    dest.writeInt(this.mTracksCount);
    dest.writeInt(this.mBookmarksCount);
    dest.writeInt(this.mTypeIndex);
    dest.writeByte(this.mIsVisible ? (byte) 1 : (byte) 0);
  }

  protected BookmarkCategory(Parcel in)
  {
    this.mId = in.readLong();
    this.mName = in.readString();
    this.mAuthor = in.readParcelable(Author.class.getClassLoader());
    this.mAnnotation = in.readString();
    this.mDescription = in.readString();
    this.mTracksCount = in.readInt();
    this.mBookmarksCount = in.readInt();
    this.mTypeIndex = in.readInt();
    this.mIsVisible = in.readByte() != 0;
  }

  public static final Creator<BookmarkCategory> CREATOR = new Creator<BookmarkCategory>()
  {
    @Override
    public BookmarkCategory createFromParcel(Parcel source)
    {
      return new BookmarkCategory(source);
    }

    @Override
    public BookmarkCategory[] newArray(int size)
    {
      return new BookmarkCategory[size];
    }
  };
}
