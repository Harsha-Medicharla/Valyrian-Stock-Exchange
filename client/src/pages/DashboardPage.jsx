import { useState } from 'react'
import { useQuery } from '@tanstack/react-query'
import api from '../api/axios'
import TopBar from '../components/TopBar'
import TabNav from '../components/TabNav'
import PortfolioSummaryBar from '../components/PortfolioSummaryBar'
import HoldingsTable from '../components/HoldingsTable'
import PriceChart from '../components/PriceChart'
import OrderForm from '../components/OrderForm'
import OrderPreview from '../components/OrderPreview'
import TradeHistoryLog from '../components/TradeHistoryLog'
import OrderBook from '../components/OrderBook'
import OpenOrdersPanel from '../components/OpenOrdersPanel'
import AdminView from '../components/AdminView'
import AnalyticsView from '../components/AnalyticsView'

export default function DashboardPage() {
  const [activeTab, setActiveTab] = useState('trader')
  const [selectedAssetIdState, setSelectedAssetId] = useState(null)
  const [lastTrade, setLastTrade] = useState(null)

  // OrderForm state lifted up so OrderPreview can do live calculations
  const [orderSide, setOrderSide] = useState('BUY')
  const [orderType, setOrderType] = useState('MARKET')
  const [orderAssetIdState, setOrderAssetId] = useState(null)
  const [orderQty, setOrderQty] = useState('')
  const [orderLimitPrice, setOrderLimitPrice] = useState('')

  // Single portfolio query shared across the whole dashboard
  const { data: portfolio, isLoading } = useQuery({
    queryKey: ['portfolio'],
    queryFn: () => api.get('/api/portfolio').then((r) => r.data),
    staleTime: 30_000,
  })

  const { data: assets = [] } = useQuery({
    queryKey: ['assets'],
    queryFn: () => api.get('/api/assets').then((r) => r.data),
    staleTime: 30_000,
  })

  const selectedAssetId = selectedAssetIdState ?? assets[0]?.assetId ?? null
  const orderAssetId = orderAssetIdState ?? assets[0]?.assetId ?? null

  function handleTradeSuccess(result, side, asset) {
    setLastTrade({ ...result, side, symbol: asset.symbol, qty: orderQty })
  }

  return (
    <div className="min-h-screen bg-bg">
      <TopBar />

      <div className="pt-14">
        <TabNav activeTab={activeTab} onTabChange={setActiveTab} />

        <main className="p-6 space-y-6">
          {/* ── TRADER TAB ── */}
          {activeTab === 'trader' && (
            <>
              {/* Summary bar */}
              <PortfolioSummaryBar />

              {/* Holdings + Chart (2/3 + 1/3) */}
              <div className="grid grid-cols-1 xl:grid-cols-3 gap-6">
                <div className="xl:col-span-2">
                  <HoldingsTable
                    selectedAssetId={selectedAssetId}
                    onSelectAsset={(id) => {
                      setSelectedAssetId(id)
                      setOrderAssetId(id)
                    }}
                  />
                </div>
                <div>
                  <PriceChart selectedAssetId={selectedAssetId} assets={assets} />
                </div>
              </div>

              {/* Order Form + Order Preview */}
              <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
                <OrderForm
                  assets={assets}
                  isLoading={isLoading}
                  side={orderSide}
                  orderType={orderType}
                  assetId={orderAssetId}
                  qty={orderQty}
                  limitPrice={orderLimitPrice}
                  onSideChange={setOrderSide}
                  onOrderTypeChange={setOrderType}
                  onAssetChange={(id) => {
                    setOrderAssetId(id)
                    setSelectedAssetId(id)
                  }}
                  onQtyChange={setOrderQty}
                  onLimitPriceChange={setOrderLimitPrice}
                  onTradeSuccess={handleTradeSuccess}
                />
                <OrderPreview
                  side={orderSide}
                  orderType={orderType}
                  assetId={orderAssetId}
                  qty={orderQty}
                  limitPrice={orderLimitPrice}
                  lastTrade={lastTrade}
                  assets={assets}
                  portfolio={portfolio}
                />
              </div>

              {/* Order Book + Open Orders side-by-side */}
              <div className="grid grid-cols-1 xl:grid-cols-2 gap-6">
                <OrderBook selectedAssetId={selectedAssetId} assets={assets} />
                <OpenOrdersPanel />
              </div>

              {/* Trade History */}
              <TradeHistoryLog />
            </>
          )}

          {/* ── ANALYTICS TAB ── */}
          {activeTab === 'analytics' && <AnalyticsView />}

          {/* ── ADMIN TAB ── */}
          {activeTab === 'admin' && <AdminView />}
        </main>
      </div>
    </div>
  )
}
