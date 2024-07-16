package app.tourism.data.repositories

import android.content.Context
import app.tourism.data.db.Database
import app.tourism.data.remote.CurrencyApi
import app.tourism.data.remote.handleGenericCall
import app.tourism.domain.models.profile.CurrencyRates
import app.tourism.domain.models.resource.Resource
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.flow

class CurrencyRepository(
    private val api: CurrencyApi,
    private val db: Database,
    val context: Context
) {
    val currenciesDao = db.currencyRatesDao

    suspend fun getCurrency(): Flow<Resource<CurrencyRates>> = flow {
        currenciesDao.getCurrencyRates()?.let {
            emit(Resource.Success(it.toCurrencyRates()))
        }

        handleGenericCall(
            call = { api.getCurrency() },
            mapper = {
                db.currencyRatesDao.updateCurrencyRates(it.toCurrencyRatesEntity())
                it.toCurrencyRates()
            },
            context
        )
    }
}
