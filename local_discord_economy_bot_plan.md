# Local Discord Economy Bot Plan

## 1. Core Vision

This bot is a competitive economy simulator designed to run entirely inside one Discord server.

It should feel familiar to players who have used classic Discord economy bots, but it should go much deeper than repeatedly using commands such as `/work`, `/daily`, or `/gamble`.

The goal is to create a local economy where players can:

- Work jobs
- Attend college
- Build credit
- Open bank accounts
- Invest
- Trade stocks
- Own businesses
- Gamble
- Collect valuable items
- Borrow and lend money
- Become extremely wealthy
- Lose everything through terrible financial decisions

A player should realistically be able to turn $50 into $45,000 through a combination of good information, calculated risk, leverage, and luck.

At the same time, conservative players should be able to build wealth slowly through jobs, savings, education, bonds, and diversified investments.

The economy should be influenced by player activity, but it should not depend on having a large or constantly active player base.

NPC consumers, investors, banks, businesses, and market activity will keep the economy alive.

---

# 2. Server Structure

Every Discord server has its own completely separate economy.

Each server has:

- Its own players
- Its own currency
- Its own banks
- Its own businesses
- Its own stock market
- Its own job market
- Its own economic conditions
- Its own inflation rate
- Its own financial news
- Its own item economy

Nothing must be shared with another Discord server.

Cross-server multiplayer can be added later as an optional expansion, but the local economy should be fully functional by itself.

---

# 3. Local Currency

Each server has one configurable currency.

Server administrators can choose:

- Currency name
- Currency symbol
- Starting balance
- Basic income rates
- Default prices

Examples:

- Dollar
- Meisei Dollar
- Coin
- Credit
- Shrimp Buck

The currency's purchasing power can change over time through inflation and deflation.

Unlike a cross-server currency, it does not need an exchange rate against other servers. Instead, the bot tracks an internal price index.

## Currency Health

The server economy tracks:

- Money supply
- Average player wealth
- Median player wealth
- Consumer prices
- Employment
- Business revenue
- Bank stability
- Debt levels
- Investment activity
- Item supply
- Player activity

If money is created faster than goods and services are produced, prices increase.

If money becomes scarce or player spending collapses, prices may stagnate or fall.

The bot should avoid extreme inflation unless the server has genuinely created an unstable economy.

---

# 4. Player Accounts

Every player has a financial profile.

## Main Balances

- Wallet
- Checking account
- Savings account
- High-yield savings account
- Certificates of deposit
- Brokerage account
- Retirement account
- Business accounts

## Player Statistics

- Net worth
- Liquid cash
- Total debt
- Credit score
- Income
- Education
- Employment
- Investment return
- Business ownership
- Items owned
- Bankruptcy history
- Financial reputation

## Financial History

The bot records:

- Balance history
- Net-worth history
- Loan payments
- Missed payments
- Investment gains
- Investment losses
- Major purchases
- Bankruptcy
- Business failures
- Large transfers

This allows profiles to show whether a player became wealthy through work, investments, business ownership, gambling, inheritance, or reckless speculation.

---

# 5. Starter Banks

The economy begins with NPC banks so players immediately have access to financial services.

## Bank of Miyamii

Parody of Bank of America.

Characteristics:

- Stable
- Traditional
- Large
- Conservative lending
- Moderate fees
- Reliable checking and savings
- Good long-term banking option

## Horen

Parody of Chase.

Characteristics:

- Aggressive lending
- High credit limits
- Strong business banking
- Good investment integration
- More willing to approve wealthy customers

## Sisi Princeton

Parody of Wells Fargo.

Characteristics:

- Bureaucratic
- Requires excessive paperwork
- Strong mortgage rates
- Complicated account requirements
- Occasional suspicious account-related scandals

## Kitty Express

Parody of American Express.

Characteristics:

- Premium financial institution
- High rewards
- Luxury cards
- Excellent fraud protection
- Strict approval requirements
- High annual fees

## Discover

Discover remains Discover.

Characteristics:

- Beginner-friendly
- Frequently overlooked
- Strong promotions
- Good student products
- Surprisingly competitive rewards

## Capital Two

Parody of Capital One.

Characteristics:

- Friendly to new players
- Easy starter accounts
- High approval rates
- Lower beginning limits
- Strong credit-building tools

---

# 6. Bank Products

Banks can offer different versions of the following products.

## Deposit Accounts

### Checking Accounts

- Used for normal transactions
- Very low or no interest
- May include maintenance fees
- Required for debit cards

### Savings Accounts

- Low risk
- Moderate interest
- High liquidity
- Possible withdrawal limits

### High-Yield Savings Accounts

- Higher variable interest
- Rates respond to economic conditions
- Banks may reduce rates when the economy slows
- May require minimum deposits

### Certificates of Deposit

- Guaranteed return
- Money locked for a selected period
- Early withdrawal penalty
- Different terms and rates

## Credit Products

- Starter credit cards
- Student credit cards
- Cash-back cards
- Travel cards
- Business cards
- Premium cards
- Secured cards
- Personal loans
- Auto-style loans
- Mortgages
- Business loans
- Lines of credit

Each product can have:

- Interest rate
- Annual fee
- Credit limit
- Rewards
- Minimum credit score
- Income requirement
- Late fee
- Grace period

---

# 7. Credit System

Every player has a credit score and credit history.

## Credit Factors

- Payment history
- Credit utilization
- Account age
- Number of recent applications
- Current debt
- Income
- Defaults
- Bankruptcy
- Secured collateral

## Credit Effects

Credit determines:

- Card approvals
- Loan approvals
- Interest rates
- Credit limits
- Mortgage eligibility
- Business financing
- Margin access
- Insurance costs
- Rental or property access

Credit should be useful enough that players want to build it, but dangerous enough that borrowing can ruin them.

## Bankruptcy

Bankruptcy can:

- Discharge eligible debts
- Damage credit
- Seize certain assets
- Restrict borrowing temporarily
- Remain visible on the player's profile
- Allow the player to recover instead of becoming permanently trapped

Players should always retain access to basic work and starter income after bankruptcy.

---

# 8. Jobs and Employment

Jobs provide the safest starting income.

Players begin with entry-level work and unlock better jobs through:

- Education
- Experience
- Skills
- Reputation
- Certifications
- Luck
- Server roles

## Example Job Tiers

### Entry-Level

- Fast-food worker
- Cashier
- Janitor
- Delivery worker
- Warehouse worker
- Retail employee

### Skilled

- Technician
- Mechanic
- Bookkeeper
- Medical assistant
- Electrician
- Designer

### Professional

- Accountant
- Engineer
- Programmer
- Financial analyst
- Lawyer
- Manager

### Executive

- Director
- Vice president
- Chief financial officer
- Chief executive officer

## Job Mechanics

Jobs may include:

- Salary
- Work cooldown
- Performance rating
- Promotion chance
- Layoff risk
- Benefits
- Retirement contributions
- Bonuses
- Education requirements

Jobs should not be the fastest route to extreme wealth.

They provide stability, creditworthiness, and startup capital.

---

# 9. Server Role Income

Administrators can connect Discord roles to jobs or stipends.

Examples:

- Moderator
- Artist
- Developer
- Booster
- Event winner
- Staff member
- Government official

Role income can be:

- Fixed daily income
- Hourly wage
- Weekly salary
- One-time bonus
- Percentage bonus to another job

Role income should be configurable and subject to server balance limits.

---

# 10. Education System

Education improves employment opportunities and access to financial information.

It should improve consistency rather than guarantee wealth.

Players without degrees must still be able to become rich through experience, business ownership, or luck.

## Starter Colleges

### Miyamii Institute of Technology

Parody of MIT.

Strengths:

- Engineering
- Computer science
- Mathematics
- Quantitative finance
- Research

Type:

- Elite private institution
- Expensive
- Highly selective

### Princeton

Named after a character in the user's universe.

Strengths:

- Economics
- Mathematics
- Law
- Politics
- Business

Type:

- Elite private institution

### Harvard

Harvard.

Strengths:

- Law
- Business
- Medicine
- Politics
- Prestige

Type:

- Elite private institution

### Wheat University

Parody of Rice University.

Strengths:

- Engineering
- Materials science
- Agriculture
- Research
- Business

Type:

- Elite private institution

### Shrimp Community College

Strengths:

- Accounting
- Business fundamentals
- Trades
- Transfer programs

Type:

- Affordable community college
- High accessibility

### Sea Slug Community College

Strengths:

- Accounting
- Technology
- Logistics
- Certifications

Type:

- Affordable community college
- Unreasonably strong accounting program

### University of Rivamonte

Strengths:

- Business
- Engineering
- Architecture
- Economics

Type:

- Higher-end public university

### University of Florensa

Strengths:

- Strong across most majors
- Research
- Finance
- Law
- Engineering

Type:

- Elite public university

### University of Sielgrada

Strengths:

- General education
- Teaching
- Business
- Technical careers

Type:

- Mid-tier public university
- Affordable and accessible

## Degrees and Effects

### Finance

- Better stock analysis
- Better valuation estimates
- Earlier access to advanced investments
- Better risk reports

### Economics

- Better macroeconomic forecasts
- Improved inflation estimates
- Improved recession indicators

### Accounting

- Lower business waste
- Better fraud detection
- Better tax efficiency
- Improved financial statements

### Computer Science

- Unlocks market alerts
- Unlocks automated investing tools
- Improves cybersecurity
- Reduces automation costs

### Engineering

- Improves factories
- Reduces maintenance costs
- Improves production
- Unlocks advanced manufacturing businesses

### Business

- Improves company management
- Reduces administrative costs
- Improves hiring
- Improves expansion success

### Marketing

- Improves sales
- Improves customer demand
- Improves IPO interest
- Improves item listing visibility

### Psychology

- Improves negotiation
- Improves employee retention
- Improves customer satisfaction

### Law

- Improves contracts
- Improves bankruptcy outcomes
- Improves mergers
- Improves hostile-takeover defense

## Hidden Skills

Education and experience contribute to skills such as:

- Financial literacy
- Negotiation
- Leadership
- Research
- Risk assessment
- Technical skill
- Charisma
- Accounting
- Management

Skills should affect information quality and opportunity access, not secretly force successful outcomes.

A finance graduate may receive a better estimate of whether a stock is undervalued, but the stock can still fall.

---

# 11. Starter Businesses

NPC businesses provide jobs, goods, services, and stock-market activity before players create their own companies.

Starter businesses can include:

- Restaurants
- Grocery stores
- Factories
- Construction companies
- Technology firms
- Mining companies
- Logistics companies
- Banks
- Casinos
- Retail stores
- Utility companies

NPC businesses should:

- Hire players
- Pay wages
- Earn revenue
- Pay expenses
- Buy resources
- Sell products
- Take loans
- Expand
- Shrink
- Go bankrupt

---

# 12. Player-Owned Businesses

Players can create businesses after meeting requirements.

Possible requirements:

- Startup capital
- Credit score
- Business license
- Education or experience
- Initial inventory
- Location or property

## Business Features

- Business bank account
- Employees
- Payroll
- Inventory
- Revenue
- Expenses
- Debt
- Taxes
- Equipment
- Property
- Marketing
- Customer demand
- Reputation
- Expansion
- Bankruptcy

Players can choose whether to keep a company private or eventually list it on the stock market.

---

# 13. Stock Market

The server has a local stock exchange.

All listed stocks exist only inside that server.

NPC investors provide activity when player volume is low.

## Starter Companies

### MEOW

Sector:

- Technology

Personality:

- High-growth market favorite
- Expensive valuation
- Strong brand loyalty
- Sensitive to innovation and product launches

### Rat Mining

Sector:

- Mining and raw materials

Personality:

- Commodity-driven
- Cyclical
- Benefits from shortages and construction booms

### Yummy Burger

Sector:

- Consumer food

Personality:

- Stable
- Defensive
- Reliable dividends
- Sensitive to food costs and consumer spending

### Intelligent

Parody of Intel.

Sector:

- Semiconductors

Personality:

- Technically important
- Inconsistent execution
- Strong product cycles followed by manufacturing problems

### Enron

Sector:

- Energy and financial chaos

Personality:

- Suspiciously profitable
- High dividends
- Hidden fraud risk
- Occasional accounting scandals
- Potential catastrophic collapse

### Los Pollos Ermanos

Sector:

- Restaurants and logistics

Personality:

- Consistent profits
- Strong delivery network
- Occasional law-enforcement investigations
- Totally legitimate

### Skibidi Steel Co.

Sector:

- Steel and heavy industry

Personality:

- Cyclical
- Benefits from construction
- Sensitive to energy and raw-material prices

### Boogle

Sector:

- Search, advertising, cloud computing, and artificial intelligence

Personality:

- Strong growth
- Large cash reserves
- Frequent antitrust investigations
- Constant product launches

### Dogecoin Mines

Sector:

- Speculative mining

Personality:

- Meme-driven
- Extremely volatile
- Minimal fundamental value
- Driven by hype, panic, and social activity

## Stock Fundamentals

Every company tracks:

- Revenue
- Expenses
- Profit
- Cash
- Debt
- Assets
- Growth
- Dividend
- Shares outstanding
- Market capitalization
- Public sentiment
- Risk
- Liquidity
- Volatility

---

# 14. Simulated Market Engine

The market must work even with very few players.

NPC investors create buy and sell activity.

## Market Participants

- Human players
- Value investors
- Momentum traders
- Dividend investors
- Index funds
- Panic sellers
- Meme traders
- Institutional funds
- Company insiders
- Market makers

## Price Influences

Each stock's movement is influenced by:

- Fundamentals
- Player orders
- NPC orders
- News
- Economic conditions
- Sector performance
- Liquidity
- Sentiment
- Random noise

Conceptually:

```text
price_change =
    fundamental_pressure
  + player_order_pressure
  + npc_order_pressure
  + news_pressure
  + economic_pressure
  + bounded_random_noise
```

Randomness should create uncertainty, not completely control outcomes.

## Fundamental Value

Each company has:

- Estimated fundamental value
- Current market price
- Sentiment
- Uncertainty
- Liquidity
- Volatility

The market price can move above or below fundamental value.

This permits:

- Bubbles
- Crashes
- Undervalued companies
- Overvalued companies
- Slow corrections
- Speculative mania

## NPC Activity Scaling

NPC market influence changes based on player activity.

Low player activity:

- NPCs provide most trading volume

High player activity:

- NPC influence decreases
- Player orders matter more

The transition should be gradual.

---

# 15. Trading Features

The stock market can support:

- Market orders
- Limit orders
- Bid prices
- Ask prices
- Bid-ask spreads
- Partial order fills
- Trading volume
- Price charts
- Dividends
- Stock splits
- Share dilution
- Initial public offerings
- Short selling
- Margin trading
- Trading halts
- Delisting
- Bankruptcy

Advanced features can be unlocked gradually so new players are not overwhelmed.

---

# 16. Investments

Players can use more than individual stocks.

## Low-Risk

- Savings accounts
- High-yield savings accounts
- Certificates of deposit
- Government bonds

## Moderate-Risk

- Corporate bonds
- Index funds
- Real-estate funds
- Large stable stocks

## High-Risk

- Individual growth stocks
- Commodities
- Private businesses
- Collectibles

## Extreme-Risk

- Options
- Margin
- Short selling
- Penny stocks
- Dogecoin Mines
- Enron

Investments should differ in:

- Risk
- Liquidity
- Return
- Volatility
- Minimum investment
- Holding period

---

# 17. Loans and Lending

Players can borrow from NPC banks.

Possible loan types:

- Personal loan
- Student loan
- Business loan
- Mortgage
- Secured loan
- Margin loan
- Credit-card balance

Loan terms include:

- Principal
- Interest rate
- Term
- Minimum payment
- Collateral
- Late fee
- Default conditions

Player-to-player lending can be added later after the contract and anti-abuse systems are stable.

---

# 18. Gambling

Gambling remains part of the bot, but it should connect to the economy.

Games may include:

- Slots
- Blackjack
- Roulette
- Lotteries
- Prediction markets
- Simulated sports betting

## NPC Casinos

Starter casinos provide reliable access to games.

They have:

- Cash reserves
- Payout limits
- House edge
- Promotions
- Loyalty programs

## Player-Owned Casinos

Advanced players may open casinos.

Player casinos can:

- Set betting limits
- Fund reserves
- Earn house profits
- Lose money
- Become insolvent

---

# 19. Items and Collectibles

The economy includes tradable virtual items.

Items may have:

- Utility
- Limited supply
- Serial numbers
- Durability
- Production cost
- Ownership history
- Market price
- Rarity
- Income generation

Examples:

- Limited collectibles
- Equipment
- Vehicles
- Property
- Business machinery
- Decorative items
- Event trophies
- Historical launch items

Collectible values should be driven by supply and player demand rather than fixed prices.

---

# 20. Property

Players and businesses can own property.

Examples:

- Apartments
- Houses
- Stores
- Offices
- Factories
- Warehouses
- Luxury property

Property may provide:

- Storage
- Business capacity
- Rental income
- Prestige
- Collateral
- Maintenance expenses

Property values respond to local economic conditions.

---

# 21. Contracts

The bot can support standardized contracts.

Possible contracts:

- Loans
- Employment
- Revenue sharing
- Business partnerships
- Insurance
- Escrow
- Rentals
- Installment sales

Contracts must clearly display:

- Parties
- Payment amount
- Payment schedule
- Duration
- Collateral
- Penalties
- Cancellation terms

Complex player-written contracts should not be allowed until strong validation and anti-scam protections exist.

---

# 22. Financial News

The server receives automated financial news.

Possible publication name:

- The Daily Tail

## Sections

- Markets
- Banking
- Business
- Education
- Employment
- Crime
- Economic indicators
- Opinion

## Example Headlines

- Enron investigated again
- Horen raises savings rates
- Rat Mining discovers a new deposit
- Yummy Burger announces higher food costs
- Boogle faces an antitrust investigation
- University of Florensa graduates 300 finance majors
- Skibidi Steel expands production
- Dogecoin Mines rises 240% on no identifiable news

News should influence expectations and market behavior, but it should not directly dictate exact price changes.

---

# 23. Economic Events

Events should have logical consequences.

Examples:

- Recession
- Construction boom
- Commodity shortage
- Technology breakthrough
- Interest-rate increase
- Credit crisis
- Bank failure
- Consumer spending boom
- Food shortage
- Labor shortage
- Housing boom
- Housing crash
- Corporate scandal

Events may be caused by:

- Random simulation
- Player behavior
- Business failures
- Excessive debt
- Inflation
- Market bubbles
- Bank instability

---

# 24. Local Economic Simulation

The server economy tracks broad conditions.

## Main Indicators

- Gross server product
- Inflation
- Unemployment
- Average income
- Median income
- Consumer confidence
- Business confidence
- Debt
- Bank stability
- Stock-market performance
- Business formation
- Bankruptcy rate

## Economic Feedback Loops

Example recession:

1. A major company fails.
2. Employees lose jobs.
3. Spending falls.
4. Business revenue declines.
5. More layoffs occur.
6. Loan defaults increase.
7. Banks tighten lending.
8. Investment declines.
9. The recession becomes worse.

The bot should allow these conditions to emerge rather than simply activating a random “recession mode.”

---

# 25. Inflation and Money Sinks

The economy needs ways to remove money.

Possible money sinks:

- Taxes
- Transaction fees
- Loan interest
- Bank fees
- Tuition
- Property maintenance
- Business expenses
- Item repairs
- Licensing fees
- Insurance premiums
- Luxury cosmetics
- NPC auctions
- Bankruptcy fees
- Trading fees

Money sinks should feel connected to gameplay decisions.

---

# 26. Leaderboards

The server can rank players by:

- Net worth
- Cash
- Investment return
- Dividend income
- Business revenue
- Credit score
- Debt
- Largest gain
- Largest loss
- Most valuable company
- Most valuable collectible
- Most bankruptcies
- Highest education
- Largest tax payment

Historical rankings should show movement over time.

Example:

```text
@CrackheadInvestor
Rank: #42 -> #2
24-hour return: +8,940%
Starting capital: $50
Current net worth: $45,112
Primary cause: leveraged Dogecoin Mines position
```

---

# 27. Safety and Anti-Abuse

The economy should be chaotic, but technically fair.

Important protections:

- No real-money purchases for tradable assets
- Transfer limits for new accounts
- Anti-alt detection
- Rate limits
- Audit logs
- Confirmation prompts for leverage
- Trading halts
- Maximum debt limits
- Bankruptcy recovery
- Protected starter income
- Admin action logs
- No invisible admin money creation
- Clear risk labels
- Server rollback tools
- Fraud detection

Players should be allowed to lose virtual wealth, but not be permanently locked out of the game.

---

# 28. Recommended Development Order

## Phase 1: Basic Economy

Build:

- Player accounts
- Wallet
- Checking account
- Currency
- `/work`
- `/daily`
- Basic items
- Basic shop
- Transfers
- Leaderboard

## Phase 2: Banking and Credit

Build:

- Starter banks
- Savings accounts
- HYSAs
- CDs
- Credit scores
- Credit cards
- Personal loans
- Loan payments
- Defaults
- Bankruptcy

## Phase 3: Jobs and Education

Build:

- Job system
- Job requirements
- Promotions
- Colleges
- Tuition
- Degrees
- Skills
- Better job access

## Phase 4: Stock Market

Build:

- Starter companies
- Company fundamentals
- NPC investors
- Market and limit orders
- Price history
- Dividends
- Financial news

## Phase 5: Businesses

Build:

- Player-owned businesses
- Employees
- Payroll
- Inventory
- Revenue
- Expenses
- Loans
- Business failure

## Phase 6: Advanced Investing

Build:

- Bonds
- Index funds
- Commodities
- Short selling
- Margin
- Options
- Private investment

## Phase 7: Full Economic Simulation

Build:

- Inflation
- Unemployment
- Recessions
- Bank failures
- Economic events
- Consumer confidence
- Business confidence
- Emergent feedback loops

## Phase 8: Advanced Assets

Build:

- Property
- Collectibles
- Player casinos
- Contracts
- Insurance
- Business IPOs
- Hostile takeovers

---

# 29. Core Design Rules

1. The economy must work with one active player.
2. NPC activity should fill missing demand and liquidity.
3. Player behavior should still influence outcomes.
4. Randomness should create uncertainty, not meaningless chaos.
5. Education should improve information and access, not guarantee profit.
6. Jobs should provide stability, not the fastest route to wealth.
7. Investments should offer different risk and liquidity profiles.
8. Every major source of money should have corresponding money sinks.
9. Players must always have a path to recover after failure.
10. The local server should feel like a living economy even without multiplayer.
11. Grinding should be viable but less powerful than intelligent strategy.
12. The richest player should not always be the player who has played the longest.

---

# 30. Final Gameplay Goal

The finished bot should feel like a normal local Discord economy bot at first.

A new player can:

- Claim daily income
- Get a job
- Open a bank account
- Attend college
- Build credit
- Buy a few items

As they progress, they discover:

- Stocks
- Bonds
- Businesses
- Loans
- Credit cards
- Property
- Gambling
- Advanced investments
- Market manipulation
- Economic crises

Eventually, one player may own several companies, hold premium bank accounts, employ other server members, trade on margin, control a large portion of the stock market, and still be one terrible Enron earnings report away from bankruptcy.

That is the intended experience:

A self-contained Discord economy game where every server develops its own financial history, market legends, failed businesses, famous investors, and completely avoidable economic disasters.
