from sports.nba import NBAProvider
from sports.nfl import NFLProvider

# Registry of all available sport providers.
# To add a new league, create a provider class in this package and add it here.
ALL_PROVIDERS = [NBAProvider, NFLProvider]
